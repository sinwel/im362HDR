#ifndef _PROFILER_H_
#define _PROFILER_H_


extern unsigned int profiler_init(const char *filename_profiler_out, const char *filename_crc_out);

extern unsigned int profiler_start(const char *prototype, unsigned int height, unsigned int width);
extern unsigned int profiler_start(const char *prototype, unsigned int size);
extern unsigned int profiler_start();

extern unsigned int profiler_end();
extern void			profiler_crc(unsigned int crc);


#ifdef __XM4__
	#define PROFILER_START(height, width)	profiler_start(__PRETTY_FUNCTION__, height, width)
	#define PROFILER_END()					profiler_end()
#else 
	#define PROFILER_START(height, width)	profiler_start(__FUNCSIG__, height, width)
	#define PROFILER_END()					profiler_end()
#endif 
	#define PROFILER_CRC(crc)				profiler_crc(crc)

#endif // _PROFILER_H_
