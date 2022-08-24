/**
 *
 *  ___         _____
 * |   |  ____ /
 * |---  |____ |____,  ____
 * |   \ |____      |  ---- PIRE!
 * |    \      _____/
 *
 * res-pire : open source re-implementation of stupid x-pire.net
 * 
 * this file handles the whole decryption. it is such an incredible
 * mess, i immediately feel sick by just looking at it. however,
 * written in <1h and it works like a charm.
 *
 * to compile, don't forget to link:
 * -ljpeg   : libjpeg for parsing input (encrypted) image
 * -lcurl   : libcurl for querying the keyserver
 * -lcrypto : openssl for decryption
 *
 * Also, you won't notice, but this is actually C++ code. but i
 * felt like hacking so....anyways, you should compile using a
 * c++ compiler, nevertheless :P
 *
 * THIS IS A "PROTOTYPE"
 * consider this as a prototype to use as a base for further
 * development (in case there was a use for further developent...)
 *
 * Still ToDo (naa, rather not)
 * = Handle CAPTCHAs
 * = Locally store (cache) keys
 * = I think for facebook/stuff there is an additional step
 *   of reed-solomon FEC required. Whatever.
 *
 *
 **/
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <jpeglib.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <curl/curl.h>

/* -------------- JPEG STUFF -------------------- */
const static JOCTET EOI_BUFFER[1] = { JPEG_EOI };
struct my_source_mgr {
	struct jpeg_source_mgr	pub;
	const JOCTET 			*data;
	size_t       			len;
};

static void my_init_source(j_decompress_ptr cinfo) {}

static boolean my_fill_input_buffer(j_decompress_ptr cinfo) {
	my_source_mgr* src = (my_source_mgr*)cinfo->src;
	src->pub.next_input_byte = EOI_BUFFER;
	src->pub.bytes_in_buffer = 1;
	return TRUE;
}
static void my_skip_input_data(j_decompress_ptr cinfo, long num_bytes) {
	my_source_mgr* src = (my_source_mgr*)cinfo->src;
	if (src->pub.bytes_in_buffer < (unsigned long)num_bytes) {
		src->pub.next_input_byte = EOI_BUFFER;
		src->pub.bytes_in_buffer = 1;
	} else {
		src->pub.next_input_byte += num_bytes;
		src->pub.bytes_in_buffer -= num_bytes;
	}
}
static void my_term_source(j_decompress_ptr cinfo) {}

static void my_set_source_mgr(j_decompress_ptr cinfo, const char* data, size_t len) {
	my_source_mgr* src;
	if (cinfo->src == 0) { // if this is first time;  allocate memory
		cinfo->src = (struct jpeg_source_mgr *)(*cinfo->mem->alloc_small)
			((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(my_source_mgr));
	}
	src = (my_source_mgr*) cinfo->src;
	src->pub.init_source = my_init_source;
	src->pub.fill_input_buffer = my_fill_input_buffer;
	src->pub.skip_input_data = my_skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart; // default
	src->pub.term_source = my_term_source;
	// fill the buffers
	src->data = (const JOCTET *)data;
	src->len = len;
	src->pub.bytes_in_buffer = len;
	src->pub.next_input_byte = src->data;
}


/* -------------- CURL STUFF -------------------- */
struct curlreq_mem {
	char *memory;
	size_t size;
};
static size_t curlreq_mem_cb(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size*nmemb;
	struct curlreq_mem *mem = (struct curlreq_mem*)data;
	if(NULL==mem->memory) {
		mem->memory = (char*)malloc(realsize+1);
		mem->size = 0;
	} else {
		mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
	}
	if(NULL==mem->memory) {
		fprintf(stderr, "ARGH MEMORY ISSUE WITH CURL!\n");
		exit(-1);
	}
	memcpy( &(mem->memory[mem->size]), ptr, realsize );
	mem->size += realsize;
	mem->memory[mem->size] = 0;
	return realsize;
}

/* -------------- Main Code -------------------- */

int main(int argc, char* argv[]) {
	if(argc<2) {
		fprintf(stderr,"%s <filename>\n", argv[0]);
		return 1;
	}


	FILE *fd = fopen(argv[1], "r");
	if(!fd) {
		return 1;
	}

	/* completely read file to memory */
	fseek(fd, 0L, SEEK_END);
	long total_size = ftell(fd);
	fseek(fd, 0L, SEEK_SET);

	char *buffer = new char[total_size];
	long left = total_size;
	char *p = buffer;
	while(left) {
		size_t done = fread(p, 1, left, fd);
		if(0==done) {
			break;
		}
		p += done;
		left -= done;
	}
	fclose(fd);
	if(left) {

		delete [] buffer;
		return 1;
	}

	unsigned image_id = 0;
	char *server_address;
	char *encrypted_data = NULL;
	unsigned encrypted_data_length = 0;
	char *decrypted_image = NULL;
	unsigned decrypted_len = 0;

	char *key_in;

	unsigned char md[SHA256_DIGEST_LENGTH];
	unsigned char md_string[2*SHA256_DIGEST_LENGTH];

	/** let libjpeg parse **/
	{
		struct jpeg_decompress_struct cinfo;
		struct jpeg_error_mgr jerr;
		cinfo.err = jpeg_std_error(&jerr);
		jpeg_create_decompress(&cinfo);
		my_set_source_mgr(&cinfo, buffer, total_size);

		jpeg_save_markers(&cinfo, JPEG_COM, 0xFFFF);

		jpeg_read_header(&cinfo, TRUE);
		/* process markers */
		int count = 0;
		{
			jpeg_saved_marker_ptr marker;
			for(marker=cinfo.marker_list; marker!=NULL; marker=marker->next) {
				if(marker->marker == JPEG_COM) {
					switch(count++) {
						case 0: // ID
							image_id = atoi((const char*)marker->data);
							break;
						case 1: // Server-Address
							server_address = (char*)malloc(marker->data_length+1);
							memcpy(server_address, marker->data, marker->data_length);
							server_address[marker->data_length]=0;
							break;
						case 2: // unknown
							break;
						case 3: // encrypted image
							encrypted_data = (char*)malloc(marker->data_length);
							memcpy(encrypted_data, marker->data, marker->data_length);
							encrypted_data_length = marker->data_length;
							break;
					}
				} 
			}
		}
	}
	printf(">> marker_id = %d\n", image_id);
	printf(">> server = %s\n", server_address);

	/** compute hash of encrypted data **/
	{
		SHA256_CTX ctx;
		//unsigned char md[SHA256_DIGEST_LENGTH];
		SHA256_Init(&ctx);
		SHA256_Update(&ctx, encrypted_data, encrypted_data_length);
		SHA256_Final(md, &ctx);
		for(int i=0; i<SHA256_DIGEST_LENGTH; ++i) {
			printf("%02X",md[i]);
			sprintf((char*)&md_string[2*i], "%02x", md[i]);
		}
		printf("\n");
	}

	/** request key from server **/
	{
		struct curlreq_mem mtmp;
		//mtmp.memory = (char*)malloc(1);
		mtmp.memory = NULL;

		CURL *curl;
		curl_global_init(CURL_GLOBAL_ALL);
		curl = curl_easy_init();

		char url[512];
		sprintf(url, "%s/api/get_key.json?key_id=%d&image_hash=%s", server_address, image_id, md_string);
		printf(">> url->%s\n", url);

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlreq_mem_cb);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&mtmp);

		curl_easy_perform(curl);

		//printf("DONE!\n%s\n", mtmp.memory);

		/* extract key */
		/* ("parse" json) */
		char *ptr = mtmp.memory;
		while(ptr<mtmp.memory+mtmp.size && memcmp(ptr, "\"key\":", 6))
			ptr++;
		if(ptr<mtmp.memory+mtmp.size) {
			ptr += 7;
			char *ptr_start = ptr;
			char *ptr_end = ptr+1;
			while(ptr_end<mtmp.memory+mtmp.size && *ptr_end!='\"')
				ptr_end++;
			if(ptr_end>=mtmp.memory+mtmp.size) {
				printf("error parsing key from json :(\n");
				exit(-1);
			}
			unsigned len = ptr_end - ptr_start;
			key_in = (char*)malloc(len+1);
			memcpy(key_in, ptr_start, len);
			key_in[len]=0;

		} else {
			printf("Could not get key from server! :(\n");
			exit(-1);
		}

		curl_easy_cleanup(curl);
		curl_global_cleanup();
	}

	printf(">> key=%s\n", key_in);

	unsigned char aes_key[16];
	unsigned char aes_iv[16];

	/** derive AES key + decrypt **/
	{
		unsigned len = strlen(key_in);

		const EVP_CIPHER *type = EVP_aes_128_cbc();
		EVP_BytesToKey(type, EVP_sha1(), NULL, (const unsigned char*)key_in, len, 5, aes_key, aes_iv);

		unsigned char *decrypted = (unsigned char*)malloc(encrypted_data_length);
		int p_len = encrypted_data_length;
		int f_len;

		EVP_CIPHER_CTX ctx;
		EVP_CIPHER_CTX_init(&ctx);
		EVP_DecryptInit_ex(&ctx, type, NULL, aes_key, aes_iv);
		EVP_DecryptUpdate(&ctx, decrypted, &p_len, (const unsigned char*)encrypted_data, encrypted_data_length);
		EVP_DecryptFinal(&ctx, decrypted+p_len, &f_len);

		/* sanity check if JPEG file */
		unsigned SOI = *((unsigned*)decrypted);

		if(SOI==0xE0FFD8FF||SOI==0xFFD8FFE0) { // i'm cool so i care for the big endians around ;)
			printf("valid JPEG decrypted, hurray\n");
		} else {
			printf("decryption failed :(\n");
			exit(-1);
		}

		decrypted_image = (char*)decrypted;
		decrypted_len = p_len;
	}

	/** write output **/
	{
		FILE *fd = fopen("output.jpg", "w");
		char *ptrout = decrypted_image;
		size_t left = decrypted_len;

		while(left) {
			size_t w = fwrite(ptrout, 1, left, fd);
			if(!w) break;
			left -= w;
			ptrout+= w;
		}
		fclose(fd);
	}

	return 0;
}
