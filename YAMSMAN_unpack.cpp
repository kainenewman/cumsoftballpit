//Tool for extracting files from decrypted Gensou Shoujo Taisen: You data files
//Written by Yam, last updated on April 18, 2015

/*
Data structure is as follows:

5 bytes - hash identifier ("HASH1")
40 bytes - hash of the password used in the game executable (SHA1, "bV")
4 bytes (unsigned int) -  number of files in the archive
8 bytes (double) - byte position of the filelist

Then for each file:
4 bytes (unsigned int) - file size in bytes
X bytes - file data

Then for each file:
4 bytes (unsigned int) - external filename size in bytes
X bytes - external filename
4 bytes (unsigned int) - internal filename size in bytes
X bytes - internal filename
8 bytes (double) - position of the corresponding file's data

"External" filenames are the actual names of the files; "internal" filenames are the variable names used by the game.
*/

#include <stdio.h>
#include <math.h>
#include <memory.h>
#include <string.h>
#include <string>

//Holds data for each individual file
struct file {
	unsigned int datasize;
	unsigned int datapos;
	unsigned char *name;
};

//Converts 4 bytes (big endian) to unsigned int 
unsigned int bytes_to_size(unsigned char *bytes) {
	return ((bytes[3]<<24) | (bytes[2]<<16) | (bytes[1]<<8) | bytes[0]);
}

int main(int argc, char **argv) {
	FILE * inputf = fopen(argv[1], "rb");	
	//Check for input
        if (inputf == NULL) {
                printf("ERROR: No input file!\n");
                return 0;
        }


	//Option: Offset number of files (required for some data files)
	bool ofs = false;
	int ofs_files = 0;
	if (argc > 1 && argv[2] != NULL) {
		if (strcmp(argv[2], "-of") == 0) {
			FILE * offsetf = fopen(argv[3], "rb");
			if (offsetf != NULL) {
			        unsigned char *ofbytes = new unsigned char[4];
				fseek(offsetf, 45, SEEK_SET);
       				fread(ofbytes, 4, 1, offsetf);
			        ofs_files = bytes_to_size(ofbytes);
				delete ofbytes;
			} else {
				printf("ERROR: No offset file!\n");
				return 0;
			}
			fclose(offsetf);
		} else {
			printf("ERROR: Invalid option!\n");
			return 0;
		}
	}


        //Get number of files contained
        unsigned char *tfbytes = new unsigned char[4];
        fseek(inputf, 45, SEEK_SET);
        fread(tfbytes, 4, 1, inputf);
	int num_files = bytes_to_size(tfbytes);
	delete tfbytes;

        //Create array of structs for data
	unsigned char *fnames = new unsigned char[num_files];
	//unsigned char 
        file *files = new file[num_files];

	//Get data positions
	fseek(inputf, 8, SEEK_CUR);
	for (int i=0; i<num_files; i++) {
		//Get size of next file
                unsigned char *fsbytes = new unsigned char[4];
                fread(fsbytes, 4, 1, inputf);
                unsigned int ext_size = bytes_to_size(fsbytes);
		
		files[i].datasize = ext_size;
		files[i].datapos = ftell(inputf);
		//printf("File at %u\n", files[i].datapos);
		
		fseek(inputf, ext_size, SEEK_CUR);	
		delete fsbytes;
	}


        //Get filenames
	unsigned int fnpos = ftell(inputf);
        for (int i=0; i<num_files+ofs_files; i++) {
                //Get length of next name
                unsigned char *fnlbytes = new unsigned char[4];
                fread(fnlbytes, 4, 1, inputf);
		unsigned int fname_size = bytes_to_size(fnlbytes);
		
		//Offset the file list
		if (i >= ofs_files) {
			files[i-ofs_files].name = new unsigned char[fname_size+1];
			fread(files[i-ofs_files].name, fname_size, 1, inputf);
			files[i-ofs_files].name[fname_size] = '\0';
		} else {
			fseek(inputf, fname_size, SEEK_CUR);
		}
	
                unsigned char *bfnbytes = new unsigned char[4];
                fread(bfnbytes, 4, 1, inputf);

		unsigned int skip_size = bytes_to_size(bfnbytes);
                fseek(inputf, skip_size, SEEK_CUR);
                fseek(inputf, 8, SEEK_CUR);

                delete fnlbytes;
                delete bfnbytes;
        }
	unsigned int fnsize = ftell(inputf) - fnpos;
	

	//Extract files 
	for (int i=0; i<num_files; i++) { 
		//Make sure the file doesn't already exist
		FILE * t_outputf = fopen(reinterpret_cast<char*>(files[i].name), "r");
		if (t_outputf != NULL) {	
			printf("WARNING: %s already exists! Overwrite? [y/N] ", reinterpret_cast<char*>(files[i].name));
			char ovr = getchar();
			if (ovr != 'y' && ovr != 'Y') {
 				fclose(t_outputf);
				continue;
			} 
		} 
		
		//Write data
		FILE * outputf = fopen(reinterpret_cast<char*>(files[i].name), "wb");
		unsigned char *tempdata = new unsigned char[files[i].datasize];

		fseek(inputf, files[i].datapos, SEEK_SET);
		fread(tempdata, files[i].datasize, 1, inputf);
		fwrite(tempdata, files[i].datasize, 1, outputf);
		
		fclose(outputf);
		delete tempdata;
	}


	//Write file info
	std::string dname = reinterpret_cast<char*>(argv[1]) + (std::string)".gsty";
	FILE * infof = fopen(dname.c_str(), "wb");

	//Header: GSTY
	unsigned char header[4] = {
		0x47, 0x53, 0x54, 0x59
	};
	fwrite(header, 4, 1, infof);

	unsigned char *ofdata = new unsigned char[4];
	for (int i=0; i<4; i++) {
		ofdata[i] = ofs_files >> i*8;
	}
	fwrite(ofdata, 4, 1, infof);
	
	unsigned char *hddata = new unsigned char[49]; //File hash + number of files
	fseek(inputf, 0, SEEK_SET);
	fread(hddata, 49, 1, inputf);
	fwrite(hddata, 49, 1, infof);	
	delete hddata;

	unsigned char *ftdata = new unsigned char[fnsize];
	fseek(inputf, fnpos, SEEK_SET);
	fread(ftdata, fnsize, 1, inputf);	
	fwrite(ftdata, fnsize, 1, infof);
	delete ftdata;

	fclose(infof);

	printf("Successfully extracted %u files!\n", (int)num_files);
			
	delete files;
	fclose(inputf);
	return 0;
}
