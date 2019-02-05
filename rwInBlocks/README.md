# Read and Write to SD card in Blocks 

this program uses the sdmmc api to open, write to, read from and close an SD
card using commands that treat it like memory, not disk. It's fast and
deterministic.

Note: blocks written to the disk may overwrite the VFS and therefore make the
disk unreadable on a PC. B/c this is a block write w/o a file table blocks
written cannot be read on a PC other than by a disk imaging program.
