#ifndef _CINTERFACE_H

	#define _CINTERFACE_H
	#define _HEADER_VERSION 4

	#ifdef _USE_TCC_HEADERS
		#include <tcc/math.h>
	#else
		#include <math.h>
	#endif
	#ifdef _MSC_VER
		#define PACKED __declspec(align(4))
		#define APE_API _cdecl
		#define APE_API_VARI _cdecl
		#ifndef __cplusplus
				#define inline _inline
		#endif
	#else
		#define APE_API 	__attribute__ ((cdecl))
		#define APE_API_VARI 	__attribute__ ((cdecl))
		#define PACKED 		__attribute__ ((packed))
	#endif
	#ifndef __cplusplus
		typedef unsigned char bool;
	#endif
	typedef long long timer;
	#ifdef __SIZE_TYPE__
		typedef __SIZE_TYPE__ _size_t;
	#else
		typedef size_t _size_t;
	#endif
	struct PluginData;
	struct CSharedInterface;
	typedef int VstInt32;

	typedef float ( * scaleCB)(float, float, float);

	#ifdef _DOUBLE_PROCESSING
		typedef double VstFloat;
	#else
		typedef float VstFloat;
	#endif

	#define prefix __imp__
	#define member_func(return , conv, name, args) return (conv * __imp__ ## name) args
	#define make_member_params(...) (struct  CSharedInterface * const  __this, __VA_ARGS__)
	#define make_empty_params() (struct CSharedInterface * const __this)

	PACKED struct CSharedInterface
	{
		member_func(float, APE_API, getSampleRate,
			make_empty_params());
		#define getSampleRate() __imp__ ##  getSampleRate(iface)

		member_func(int, APE_API_VARI, printLine,
			make_member_params(unsigned nColor, const char * fmt, ...));
		#define printLine(...) __imp__ ##  printLine(iface, __VA_ARGS__)

		member_func(int, APE_API, msgBox,
			make_member_params(const char * text, const char * title, int nStyle, int nBlocking));
		#define msgBox(...) __imp__ ##  msgBox(iface, __VA_ARGS__)

		member_func(enum Status, APE_API, setStatus,
			make_member_params(enum Status status));
		#define setStatus(...) __imp__ ##  setStatus(iface, __VA_ARGS__)

		member_func(int, APE_API, createKnob,
			make_member_params(const char * name, float * val, int type));
		#define createKnob(...) __imp__ ##  createKnob(iface, __VA_ARGS__)

		member_func(timer, APE_API, timerGet,
			make_empty_params());
		#define timerGet(...) __imp__ ##  timerGet(iface)

		member_func(double, APE_API, timerDiff,
			make_member_params(timer tStart));
		#define timerDiff(...) __imp__ ##  timerDiff(iface, __VA_ARGS__)

		member_func(void *, APE_API, alloc,
			make_member_params(_size_t nSize));
		#define alloc(...) __imp__ ##  alloc(iface, __VA_ARGS__)

		member_func(void, APE_API, free,
			make_member_params(void * ptr));
		#define free(...) __imp__ ##  free(iface, __VA_ARGS__)

		member_func(int, APE_API, createKnobEx,
			make_member_params(const char * name, float * val, const char * values, const char * unit));
		#define createKnobEx(...) __imp__ ##  createKnobEx(iface, __VA_ARGS__)

		member_func(void, APE_API, setInitialDelay,
			make_member_params(VstInt32 samples));
		#define setInitialDelay(...) __imp__ ##  setInitialDelay(iface, __VA_ARGS__)

		member_func(int, APE_API_VARI, createLabel,
			make_member_params(const char * name, const char * fmt, ...));
		#define createLabel(...) __imp__ ##  createLabel(iface, __VA_ARGS__)

		member_func(int, APE_API, getNumInputs,
			make_empty_params());
		#define getNumInputs(...) __imp__ ##  getNumInputs(iface)

		member_func(int, APE_API, getNumOutputs,
			make_empty_params());
		#define getNumOutputs(...) __imp__ ##  getNumOutputs(iface)

		member_func(int, APE_API, createMeter,
			make_member_params(const char * name, float * val));
		#define createMeter(...) __imp__ ##  createMeter(iface, __VA_ARGS__)

		member_func(int, APE_API, createToggle,
			make_member_params(const char * name, float * val));
		#define createToggle(...) __imp__ ##  createToggle(iface, __VA_ARGS__)

		member_func(double, APE_API, getBPM,
			make_empty_params());
		#define getBPM(...) __imp__ ##  getBPM(iface)

		member_func(float, APE_API, getCtrlValue,
			make_member_params(int ID));
		#define getCtrlValue(...) __imp__ ##  getCtrlValue(iface, __VA_ARGS__)

		member_func(void, APE_API, setCtrlValue,
			make_member_params(int ID, float val));
		#define setCtrlValue(...) __imp__ ##  setCtrlValue(iface, __VA_ARGS__)

		member_func(int, APE_API, createPlot,
			make_member_params(const char * name, float * vals, unsigned numVals));
		#define createPlot(...) __imp__ ##  createPlot(iface, __VA_ARGS__)

		member_func(int, APE_API, createRangeKnob,
			make_member_params(const char * name, const char * unit, float * val, scaleCB scale, float min, float max));
		#define createRangeKnob(...) __imp__ ##  createRangeKnob(iface, __VA_ARGS__)

		struct PluginData * data;
		void * parent;
	};

	#undef member_func
	#undef make_member_params
	#undef make_empty_params
	#undef prefix
	
	struct dummy { int x; } check;

	typedef struct CSharedInterface instance_t;

	enum {
		left = 0,
		right = 1
		#ifndef __cplusplus
			,false = 0,
			true = 1
		#endif
	};
	#ifndef __cplusplus
		enum {
			throw,
			class,
			public,
			private,
			reinterpret_cast,
			static_cast,
			dynamic_cast,
			const_cast,
			catch,
			template,
			try,
			typeid,
			typename,
			virtual,
			protected,
			namespace,
			new,
			delete,
			explicit,
			friend,
			operator
		};
	#endif
	struct knob_type_t {
		int percent;
		int hertz;
		int decibel;
		int fpoint;
		int ms;
	};

	const struct knob_type_t knobType = {0, 1, 2, 3, 4};

	#define instance instance_t *
	#define self (*_this)
	#define api (*iface)
	#define onLoad() 				onLoad(struct PluginData * _this, instance_t * iface)
	#define processReplacing(...) 	processReplacing(struct PluginData * _this, instance_t * iface, __VA_ARGS__)
	#define onUnload() 				onUnload(struct PluginData * _this, instance_t * iface)
	#define onEvent(...)			onEvent(struct PluginData * _this, instance_t * iface, __VA_ARGS__)

	#define GlobalData(program_name) struct _program_info _global_data =  { sizeof( struct PluginData), _HEADER_VERSION, program_name,0 };
	PACKED struct _program_info {
		_size_t allocSize;
		_size_t version;
		const char * name;
		int wantAllocation;
		void * (APE_API *palloc)(void*);
		void * (APE_API *pfree)(void*);
		
	};
	extern struct _program_info _global_data;
	struct _program_info * getProgramInfo() { return &_global_data;}
	#define ArraySize(x) (sizeof(x) / sizeof(x[0]))
	#define sgn(x) ((0.0 < x) - (x < 0.0))

	struct MsgButton {
		signed yes, no, retry, tryagain, con, cancel;
	};

	struct MsgStyle {
		signed ok, yesnocancel, contrycancel;

	};

	struct MsgIcon {
		signed stop, question, info, warning;
	};

	struct MsgBoxStyle {
		struct MsgIcon icon;
		struct MsgButton button;
		struct MsgStyle style;
	};

	struct Color {
		unsigned black, grey, blue, green, red;
	};

	const struct Color color = {0xFF000000, 0xFF7F7F7F, 0xFFFF0000, 0xFF00FF00, 0xFF0000FF};
	const struct MsgBoxStyle mbs = { { 16, 32, 64, 48 }, { 6, 7, 4, 10, 11, 2 }, { 0, 3, 6 }  };


	enum Status {
		STATUS_OK = 0,		// operation completed succesfully
		STATUS_ERROR = 1,	// operation failed, state errornous
		STATUS_WAIT = 2,	// operation not completed yet
		STATUS_SILENT = 3,	// the plugin should not process data
		STATUS_READY = 4,	// ready for any operation
		STATUS_DISABLED = 5,// plugin is disabled
		STATUS_HANDLED = 6  // plugin handled request, host shouldn't do anything.
	};

	struct status_t {
		enum Status ok;
		enum Status error;
		enum Status wait;
		enum Status silent;
		enum Status ready;
		enum Status disabled;
		enum Status handled;
	};
	const struct status_t status = {STATUS_OK, STATUS_ERROR, STATUS_WAIT, STATUS_SILENT, STATUS_READY, STATUS_DISABLED, STATUS_HANDLED};

	struct event_ctrlValueChanged {
		float value;
		char text[64];
		char title[64];
		VstInt32 tag;
		bool unused;
	};
	enum event_type_t
	{
		ctrlValueChanged = 0

	};
	struct eventInfo
	{
		enum event_type_t eventType;

		union {
			struct event_ctrlValueChanged * eCtrlValueChanged;

		};

	};

	struct scale_t
	{
		scaleCB exp, log, linear, polyLog, polyExp;

	};


	float linear_scale(float value, float _min, float _max) { return value * (_max - _min) + _min;}
	float poly_log_scale(float value, float _min, float _max) { return (-(value * value) + 2 * value) * (_max - _min) + _min; }
	float poly_exp_scale(float value, float _min, float _max) { return value * value * (_max - _min) + _min; }
	float exp_scale(float value, float _min, float _max) { return _min * pow(_max/_min, value); }
	float log_scale(float value, float _min, float _max) { return linear_scale(1 - value, _min, _max) - exp_scale(1 - value, _min, _max) + linear_scale(value, _min, _max);}
	const struct scale_t scale = {exp_scale, log_scale, linear_scale, poly_log_scale, poly_exp_scale};

	#define log2( n )  (log( n ) / log( 2 ))  
	#define isbadf(x) (x < x || x * 1 != x * 1 || x != x)
	#define round(r) ((r > 0.0) ? floor(r + 0.5) : ceil(r - 0.5))
	#ifndef M_PI
		#define M_PI 3.14159265358979323846
	#endif
	#ifndef PI
		#define PI M_PI
	#endif
    static const float ifs_a0 =  1.0,
           ifs_a1 = -1.666666666640169148537065260055e-1,
           ifs_a2 =  8.333333316490113523036717102793e-3,
           ifs_a3 = -1.984126600659171392655484413285e-4,
           ifs_a4 =  2.755690114917374804474016589137e-6,
           ifs_a5 = -2.502845227292692953118686710787e-8,
           ifs_a6 =  1.538730635926417598443354215485e-10;

	#define if_sin(angle, result) float ifs_x2 = angle * angle; \
		result = angle * (ifs_a0 + ifs_x2 * (ifs_a1 + ifs_x2 * \
		(ifs_a2 + ifs_x2 * (ifs_a3 + ifs_x2 * (ifs_a4 + ifs_x2 * (ifs_a5 + ifs_x2 * ifs_a6))))));

	inline float f_fmod(float x,float  y) { float a; return ((a=x/y)-(int)a)*y; }
	inline float f_sin(float angle)
	{
		float ifs_x2 = angle * angle;
		return angle * (ifs_a0 + ifs_x2 * (ifs_a1 + ifs_x2 * 
		(ifs_a2 + ifs_x2 * (ifs_a3 + ifs_x2 * (ifs_a4 + ifs_x2 * (ifs_a5 + ifs_x2 * ifs_a6))))));

	}
#endif