Commandline huffman encoder and decoder

Only useful for educational and toy purposes, for actual compression please look
at GZIP or LZMA.

However, the idea is that this is the fastest possible huffman implementation
written by yours truly

The tree encodes each byte as a character.
## Encoder output format (byte huff):

* Number of unique characters - 1 (1 byte)
* List of characters in this format:
	- character (1 byte)
	- canonical code length in bits (1 byte)
* Input bytes

This same format is decoded by the decoder, allowing you to do things like:
```
./a.out < /bin/ls | a.out -d > ls
chmod +x ./ls
./ls
```
