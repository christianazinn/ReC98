; Keep the following runtime-library segment at its original paragraph offset.
PATCH_ALIGN_TEXT segment para public 'CODE' use16
	db (16 + PATCH_CRT_PHASE) dup (90h)
PATCH_ALIGN_TEXT ends
end
