# EXEC-LOADER 

### IMPLEMENTATION

1. the program receives an executable that generates `page faults` that must be dealt with through the `segv_handler handler`
- the signal capture part is already solved in the skeleton
- upon receiving a `sig_segv signal` , the segment corresponding to the error must be searched
- I have used a function that returns the address of the found segment if it exists, or NULL otherwise
- in order to find out if the error is in a segment, it is compared with the starting value and the final value of the respective segment, iterating among all segments of the executable

2. `the segment data` is then initialized in variables (for ease and code clarity),then it is created only once, it uses flag, the custom area that represents a vector page characteristics for a segment, later being linked with the data area of ​​the segment suitable

3. follows `the mapping of the page` that caused the fault and finding out the index for the calculation subsequent
the mmap function is used which receives the offset of the page we want allocate

4. `copying the data in the segment` pages takes into account the placement of the page in the segment
It is necessary to be in the file_size area, taking into account the fact that the allocation is shared in 2 cases: 
- the simple case (in which the whole page can be loaded and it is not exceeded file_size of the segment)
- the exceptional case (in which only a certain part is loaded from the size of a page, until filling file_size ul)
in the exceptional case, it is reduced from the total size the size reserved for the other pages to extract the length the loading size of the piece

5. after the data is copied, we offer `rights book pages` to the segment it belongs to, then we `mark it in the characteristics vector` in the data area to avoid a possible repeated mapping her

### DETAILS

- the details related to the working mode can be found in the comments of the `source code`
