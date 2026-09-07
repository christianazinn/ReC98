// Included inside each KEYCFG code tail. No DOS or stdio CRT dependencies.
// Only the interrupt boundary is replaced by the host test adapter.
static int keyconfig_mem_compare(const void far *a, const void far *b, unsigned size)
{
	const uint8_t far *left = static_cast<const uint8_t far *>(a);
	const uint8_t far *right = static_cast<const uint8_t far *>(b);
	while(size--) {
		if(*left++ != *right++) return 1;
	}
	return 0;
}

static void keyconfig_mem_copy(void far *to, const void far *from, unsigned size)
{
	uint8_t far *destination = static_cast<uint8_t far *>(to);
	const uint8_t far *source = static_cast<const uint8_t far *>(from);
	while(size--) *destination++ = *source++;
}

static void keyconfig_mem_fill(void far *to, uint8_t value, unsigned size)
{
	uint8_t far *destination = static_cast<uint8_t far *>(to);
	while(size--) *destination++ = value;
}

static int keyconfig_dos_call(
	unsigned function, unsigned handle, void far *buffer, unsigned count,
	const void far *target
)
{
#if defined(KEYCONFIG_DOS_HOST_TEST)
	return keyconfig_dos_host_interrupt(function, handle, buffer, count, target);
#else
	unsigned buffer_seg = static_cast<unsigned>(
		(reinterpret_cast<unsigned long>(buffer) >> 16)
	);
	unsigned buffer_off = static_cast<unsigned>(
		reinterpret_cast<unsigned long>(buffer)
	);
	unsigned target_seg = static_cast<unsigned>(
		(reinterpret_cast<unsigned long>(target) >> 16)
	);
	unsigned target_off = static_cast<unsigned>(
		reinterpret_cast<unsigned long>(target)
	);
	unsigned result;
	unsigned failed;

	_asm {
		push ds
		push es
		push di
		mov ax, function
		mov bx, handle
		mov cx, count
		mov dx, buffer_off
		mov di, target_off
		mov ds, buffer_seg
		mov es, target_seg
		int 21h
		pop di
		pop es
		pop ds
		mov result, ax
		sbb ax, ax
		mov failed, ax
	}
	if(failed) return -static_cast<int>(result);
	if((function == 0x3E00) || (function == 0x4100) || (function == 0x5600)) return 0;
	return static_cast<int>(result);
#endif
}

static int keyconfig_dos_open(const char far *fn)
{
	return keyconfig_dos_call(0x3D00, 0, const_cast<char far *>(fn), 0, 0);
}

static bool keyconfig_dos_close(int handle)
{
	return (keyconfig_dos_call(0x3E00, handle, 0, 0, 0) >= 0);
}

static bool keyconfig_file_read_exact(const char far *fn, void far *out, unsigned size)
{
	int handle = keyconfig_dos_open(fn);
	uint8_t extra;
	bool ok;

	if(handle < 0) return false;
	ok = (
		(keyconfig_dos_call(0x3F00, handle, out, size, 0) == size) &&
		(keyconfig_dos_call(0x3F00, handle, &extra, 1, 0) == 0)
	);
	if(!keyconfig_dos_close(handle)) ok = false;
	return ok;
}

#if defined(KEYCONFIG_DOS_WRITE)
static bool keyconfig_dos_remove(const char far *fn)
{
	int result = keyconfig_dos_call(0x4100, 0, const_cast<char far *>(fn), 0, 0);
	return ((result >= 0) || (result == -2)); // Missing file is already removed.
}

static bool keyconfig_dos_rename(const char far *from, const char far *to)
{
	return (keyconfig_dos_call(0x5600, 0, const_cast<char far *>(from), 0, to) >= 0);
}

static bool keyconfig_file_replace(
	const char far *fn, const char far *temp, const char far *backup,
	const void far *data, unsigned size
)
{
	int handle;
	bool had_old;
	bool ok;

	// An inaccessible destination must never be mistaken for a missing file.
	handle = keyconfig_dos_open(fn);
	if((handle < 0) && (handle != -2)) return false;
	had_old = (handle >= 0);
	if(had_old && !keyconfig_dos_close(handle)) return false;
	if(!keyconfig_dos_remove(temp)) return false;
	handle = keyconfig_dos_call(0x3C00, 0, const_cast<char far *>(temp), 0, 0);
	if(handle < 0) return false;
	ok = (keyconfig_dos_call(0x4000, handle, const_cast<void far *>(data), size, 0) == size);
	if(!keyconfig_dos_close(handle)) ok = false;
	if(!ok || !keyconfig_dos_remove(backup)) {
		keyconfig_dos_remove(temp);
		return false;
	}
	if(had_old && !keyconfig_dos_rename(fn, backup)) {
		keyconfig_dos_remove(temp);
		return false;
	}
	if(!keyconfig_dos_rename(temp, fn)) {
		if(had_old) keyconfig_dos_rename(backup, fn);
		keyconfig_dos_remove(temp);
		return false;
	}
	keyconfig_dos_remove(backup);
	return true;
}
#endif
