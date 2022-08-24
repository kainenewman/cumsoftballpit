#!/usr/bin/python -OO
'adaptation of simpleminer for scrypt'
import sys, os, time, json, hashlib, struct, re, httplib, ast, base64
# if fails import of scrypt, you need to: sudo pip install scrypt
import multiprocessing, select, random, scrypt
TIMEOUT = 120  # seconds to wait for server response
START_TIME = time.time()
PERSISTENT = {'solved': False}  # global for storing settings
CONFIGFILE = os.getenv('SIMPLEMINER_CONFIG', None) or \
 os.path.expanduser('~/.litecoin/litecoin.conf')
MULTIPLIER = int(os.getenv('SIMPLEMINER_MULTIPLIER', '1'))
THREADS = multiprocessing.cpu_count() * MULTIPLIER
INT_SIZE = 4
HEX_INT_SIZE = INT_SIZE * 2
HEADER_SIZE = 80
HEX_HEADER_SIZE = HEADER_SIZE * 2
MAX_SECONDS = 60 if __debug__ else 20
MAX_GETWORK_FAILS = 5
ALWAYS = True
# TEST from litecoin block 29255, see https://litecoin.info/Scrypt
TEST_HEADER = \
 '01000000f615f7ce3b4fc6b8f61e8f89aedb1d0852507650533a9e3b10b9bbcc' \
 '30639f279fcaa86746e1ef52d3edb3c4ad8259920d509bd073605c9bf1d59983' \
 '752a6b06b817bb4ea78e011d012d59d4'
TEST_TARGET = \
 '000000018ea70000000000000000000000000000000000000000000000000000'
TEST_NONCE = 3562614017
def key_value(line):
 'parse key and value from configuration line'
 match = re.match('^(\w+)\s*=\s*(\S+)', line)
 return match.groups() if match else None
def parse_config(config_file):
 input = open(config_file)
 settings = dict(filter(None, map(key_value, input.readlines())))
 input.close()
 debug('settings: %s' % settings)
 return settings
def debug(message, always = False):
 if __debug__ or always:
  if message:
   print >>sys.stderr, message
def establish_connection():
 PERSISTENT['rpcserver'] = httplib.HTTPConnection(
  PERSISTENT['settings']['rpcconnect'],
  PERSISTENT['settings']['rpcport'], False, TIMEOUT)
 PERSISTENT['rpcserver'].set_debuglevel(__debug__ or 0)
def init():
 PERSISTENT['settings'] = parse_config(CONFIGFILE)
 PERSISTENT['authorization'] = base64.b64encode('%s:%s' % (
  PERSISTENT['settings']['rpcuser'], PERSISTENT['settings']['rpcpassword']))
 debug('settings now: %s' % PERSISTENT)
def rpc(method, parameters = []):
 debug('making rpc call with parameters = %s' % parameters)
 rpc_call = {'version': '1.1', 'method': method, 'id': 0, 'params': parameters}
 try:
  establish_connection()
  PERSISTENT['rpcserver'].request('POST', '/', json.dumps(rpc_call),
   {'Authorization': 'Basic %s' % PERSISTENT['authorization'],
    'Content-type': 'application/json'})
  response = PERSISTENT['rpcserver'].getresponse()
  message = response.read()
  response_object = json.loads(message)
  response.close()
 except:
  response_object = {'error': 'No response or null response', 'result': None}
  if __debug__: raise
 debug(response_object.get('error', None))
 return response_object
def getwork(data = None):
 if os.getenv('SIMPLEMINER_FAKE_DATA', False):
  if not data:
   debug('***WARNING*** this is static test data, not from server!')
   work = {'result': {
    'data': bufreverse(pad(TEST_HEADER.decode('hex'))).encode('hex'),
    'target': TEST_TARGET.decode('hex')[::-1].encode('hex'),
    'algorithm': 'scrypt:1024,1,1',
   }}
  else:
   debug('getwork called with data %s' % repr(data))
   work = {}
 else:
  work = rpc('getwork', data)
  debug('result of getwork(): %s' % work, ALWAYS)
 return work.get('result', None)
def miner_thread(thread_id, work, pipe):
 hashes, found = 0, False
 thread_start = time.time()
 debug('thread %d running bruteforce for %d seconds with random nonces' % (
  thread_id, MAX_SECONDS))
 while time.time() < thread_start + MAX_SECONDS:
  nonce = random.getrandbits(32)
  data = work + struct.pack('<I', nonce)
  hashed = PERSISTENT['hash'](data)
  hashes += 1
  if hashed.endswith(PERSISTENT['check_for']):
   found = True
   pipe.send(nonce)
   debug('thread %d found possible nonce 0x%08x after %d reps' % (
    thread_id, nonce, hashes), ALWAYS)
 pipe.send((hashes, thread_id))
 return
def bufreverse(data = None):
 '''\
 reverse groups of 4 bytes in arbitrary string of bits

 >>> bufreverse('123423453456456756786789')
 '432154326543765487659876'
 '''
 if data is None:
  return None
 length = len(data) / INT_SIZE
 return struct.pack('>%dI' % length, *(struct.unpack('<%dI' % length, data)))
def sha256d_hash(data):
 'return block hash as a little-endian 256-bit number encoded as a bitstring'
 hashed = hashlib.sha256(hashlib.sha256(data).digest()).digest()
 return hashed
def scrypt_hash(data):
 hashed = scrypt.hash(data, data, 1024, 1, 1, 32)
 return hashed
def check_hash(data = TEST_HEADER.decode('hex'), target = None, nonce = None):
 if nonce is None:
  nonce = struct.unpack('<I', data[-INT_SIZE:])[0]
 else:
  data = data[:HEADER_SIZE - INT_SIZE] + struct.pack('<I', nonce)
 if target and PERSISTENT.get('hash', None):
  check_hash = PERSISTENT['hash'](data)[::-1]  # convert to big-endian
  debug('comparing:\n %s nonce 0x%08x to\n %s' % (
   check_hash.encode('hex'), nonce, target.encode('hex')), ALWAYS)
  return check_hash < target
 else:
  print 'header: %s, nonce: 0x%08x (%d)' % (data.encode('hex'), nonce, nonce)
  print 'sha256: %s' % sha256d_hash(data)[::-1].encode('hex')
  print 'scrypt: %s' % scrypt_hash(data)[::-1].encode('hex')
def simpleminer():
 if THREADS < 4:
  raise Exception('must have at least four threads for this to work')
 init()
 consecutive_errors = 0
 while True:
  start_time = time.time()
  work = getwork()
  if not work:
   consecutive_errors += 1
   if consecutive_errors == MAX_GETWORK_FAILS:
    raise Exception('too many getwork() errors, has daemon crashed?')
   else:
    print >>sys.stderr, 'waiting for work'
    time.sleep(5)
    continue
  else:
   consecutive_errors = 0
  data = bufreverse(work['data'].decode('hex'))[:HEADER_SIZE - INT_SIZE]
  target = work['target'].decode('hex')[::-1]
  algorithm = work.get('algorithm', 'sha256d')
  if algorithm == 'scrypt:1024,1,1':
   PERSISTENT['hash'] = scrypt_hash
   PERSISTENT['check_for'] = '\0\0\0'
  elif algorithm == 'sha256d':
   PERSISTENT['hash'] = sha256d_hash
   PERSISTENT['check_for'] = '\0\0\0\0'
  else:
   raise Exception('unknown algorithm: %s' % algorithm)
  debug('work: %s' % data.encode('hex'), ALWAYS)
  debug('target: %s' % target.encode('hex'), ALWAYS)
  pipe_list = []
  total_hashes, done = 0, 0
  for thread_id in range(THREADS):
   parent_end, child_end = multiprocessing.Pipe()
   thread = multiprocessing.Process(
    target = miner_thread, args = (thread_id, data, child_end))
   thread.start()
   pipe_list.append(parent_end)
  debug('%d mining threads started' % THREADS)
  while done < THREADS:
   readable = select.select(pipe_list, [], [])[0]
   for pipe in readable:
    nonce = pipe.recv()
    debug('received: %s' % repr(nonce))
    if type(nonce) in (int, long):
     debug('checking hash for nonce 0x%08x' % nonce)
     if check_hash(data, target, nonce):
      PERSISTENT['solved'] = True
      getwork([work['data'][:HEX_HEADER_SIZE - HEX_INT_SIZE] + \
       struct.pack('>I', nonce).encode('hex') + \
       work['data'][HEX_HEADER_SIZE:]])
     else:
      debug('nonce %08x failed threshold' % nonce)
    else:
     hashes, thread_id = nonce
     total_hashes += hashes
     debug('thread %d finished' % thread_id)
     done += 1
  debug('threads finished')
  delta_time = time.time() - start_time
  debug('Combined HashMeter: %d hashes in %.2f sec, %d Khash/sec' % (
    total_hashes, delta_time, (total_hashes / 1000) / delta_time), ALWAYS)
  while multiprocessing.active_children():
   time.sleep(0.1)  # joins finished processes
  if os.getenv('SIMPLEMINER_FAKE_DATA', False) and PERSISTENT['solved']:
   break  # for timing and/or profiling
 return 'done'
def pad(message = ''):
 '''
 pad a message out to 512 bits (64 bytes)

 append the bit '1' to the message
 append k bits '0', where k is the minimum number >= 0 such that the
 resulting message length (in bits) is 448 (modulo 512).
 append length of message (before pre-processing), in bits, as 64-bit
 big-endian integer
 >>> len(pad('x' * (64 - 9)))
 64
 >>> len(pad('x' * (64 - 8)))
 128
 '''
 length = len(message)
 # 64 bytes is 512 bits; 9 is minimum padding we need for count plus 1-bit
 padding_needed = 64 - (length % 64)
 padding_needed += 64 * (padding_needed < 9)
 bit_length = length * 8
 packed_length = struct.pack('>2I', bit_length / 0x100000000, bit_length)
 padding = '\x80' + '\0' * (padding_needed - 9)
 padding += packed_length
 return message + padding
if __name__ == '__main__':
 command = os.path.splitext(os.path.basename(sys.argv[0]))[0]
 print 'exit status: %s' % eval(command)(*sys.argv[1:])
