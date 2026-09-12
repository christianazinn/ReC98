struct pf_header_t {
	uint8_t type[2]; // PF_TYPE_COMPRESSED if RLE-compressed
	int8_t aux; // Always 3, unused
	char fn[PF_FN_LEN];
	int32_t packsize;
	int32_t orgsize;
	int32_t offset; // of the file data within the entire archive
	int32_t reserved; // Always zero
};
