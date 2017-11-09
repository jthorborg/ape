#ifndef _APE_H
	#define _APE_H
	#include <stdlib.h>
	#include <assert.h>

	#define EXPORTED __declspec(dllexport)
	#define STD_API _cdecl
	#define APE_API STD_API
	#define APE_API_VARI _cdecl

	typedef char char_t;
	struct CEvent;
	typedef void (STD_API * errorFunc_t)	(void*, const char *);
	typedef int VstInt32;

	enum CodeState
	{
		None,
		Compiled,
		Initialized,
		Disabled,
		Activated,
		Released

	};

	struct CSharedInterface
	{
		float		(APE_API * getSampleRate)	(CSharedInterface * iface);
		int			(APE_API_VARI * printLine)	(CSharedInterface * iface, unsigned nColor, const char * fmt, ...);
		int			(APE_API * msgBox)			(CSharedInterface * iface, const char * text, const char * title, int nStyle, int nBlocking);
		enum Status (APE_API * setStatus)		(CSharedInterface * iface, enum Status status);
		int			(APE_API * createKnob)		(CSharedInterface * iface, const char * name, float * extVal, int type);
		long long	(APE_API * timerGet)		(CSharedInterface * iface);
		double		(APE_API * timerDiff)		(CSharedInterface * iface, long long time);
		void *		(APE_API * alloc)			(CSharedInterface * iface, size_t size);
		void		(APE_API * free)			(CSharedInterface * iface, void * ptr);
		int			(APE_API * createKnobEx)	(CSharedInterface * iface, const char * name, float * extVal, char * values, char * unit);
		void		(APE_API * setInitialDelay)	(CSharedInterface * iface, VstInt32 samples);
		int			(APE_API_VARI * createLabel)(CSharedInterface * iface, const char * name, const char * fmt, ...);
		int			(APE_API * getNumInputs)	(CSharedInterface * iface);
		int			(APE_API * getNumOutputs)	(CSharedInterface * iface);
		int			(APE_API * createMeter)		(CSharedInterface * iface, const char * name, float * extVal);
		int			(APE_API * createToggle)	(CSharedInterface * iface, const char * name, float * extVal);
		double		(APE_API * getBPM)			(CSharedInterface * iface);
		float		(APE_API * getCtrlValue)	(CSharedInterface * iface, int iD);
		void		(APE_API * setCtrlValue)	(CSharedInterface * iface, int iD, float value);
		void * plugin_data;
		const void * const engine;
		const void * const csys;
	};


	// Status definitions for operation and states.
	enum Status {
		STATUS_OK = 0,		// operation completed succesfully
		STATUS_ERROR = 1,	// operation failed, state errornous
		STATUS_WAIT = 2,	// operation not completed yet
		STATUS_SILENT = 3,	// the plugin should not process data
		STATUS_READY = 4,	// ready for any operation
		STATUS_DISABLED = 5,// plugin is disabled
		STATUS_HANDLED = 6,  // plugin handled request, host shouldn't do anything.
		STATUS_NOT_IMPLEMENTED = 7 // operation not supported
	};
	/*
		the c-state's function pointers.
	*/
	struct event_ctrlValueChanged {
		float value;
		char text[64];
		char title[64];
		VstInt32 tag;
		bool unused;
	};

	struct CEvent
	{
		// poor attempt at c polymorphism
		enum event_type_t : VstInt32 
		{
			ctrlValueChanged = 0



		} event_type;

		union event
		{
			event_ctrlValueChanged * eCtrlValueChanged;
			
		};

	};

	// pack this
	struct CProject
	{
		/*
			A language string that uniquely identifies a corrosponding compiler as set
			in the settings (name of language group)
		*/
		char_t * languageID;
		/* 
			Associated compiler (see CodeGenerator::compileProject)
			Not to be deleted!
		*/
		void * compiler;
		/*
			Interface to the API
		*/
		CSharedInterface * iface;
		/*
			Version of the engine of this program
		*/
		unsigned engineVersion;
		/*
			Special case: Program consists of a single source strings, instead of files.
			If this is set to non-zero, sourceString will point to a valid string that 
			contains the source. If not, sourceString is not valid.
		*/
		unsigned isSingleString;
		/*
			amount of files contained in the array files
		*/
		unsigned int nFiles;
		/*
			an instance-unique ID for this current project
		*/
		unsigned int uniqueID;
		/*
			See the definition. Defines state of project.
		*/
		CodeState state;
		/*
			If isSingleString is set, this points to valid source.
		*/
		char_t * sourceString;
		/*
			Array of files in this project (source files)
		*/
		char_t ** files;
		/*
			Name of this project
		*/
		char_t * projectName;
		/*
			Directory of the project. Directory + \ + projectName must yield the valid path.
		*/
		char_t * workingDirectory;
		/*
			Root directory of APE
		*/
		char_t * rootPath;
		/*
			Compiler specific arguments and switches (like commandline arguments).
			Ïntended that it be set in the config.
		*/
		char_t * arguments;
		/*
			The compiler or runtime can keep it's instance data here.
		*/
		void * userData;
	};
#ifdef __cplusplus
	extern  "C" 
	{
#endif
	EXPORTED void * STD_API GetSymbol		(CProject * p, char * s);
	EXPORTED Status STD_API CompileProject	(CProject * p, void * op, errorFunc_t e);
	EXPORTED Status STD_API SetErrorFunc	(CProject * p, void * op, errorFunc_t e);
	EXPORTED Status STD_API ReleaseProject	(CProject * p);
	EXPORTED Status STD_API InitProject		(CProject * p);
	EXPORTED Status STD_API ActivateProject	(CProject * p);
	EXPORTED Status STD_API DisableProject	(CProject * p, int didMisbehave);
	EXPORTED Status STD_API GetState		(CProject * p);
	EXPORTED Status STD_API AddSymbol		(CProject * p, const char * name, void * mem);
	EXPORTED Status	STD_API ProcessReplacing(CProject * p, float ** in, float ** out, int sampleFrames);
	EXPORTED Status	STD_API OnEvent			(CProject * p, CEvent * e);
#ifdef __cplusplus
	}
#endif
	#ifndef _APESUPPORT_ISNT_COMPILER
		inline void _impcheck()
		{
			/*
				this function exists to ensure the above functions are actually implemented.
				crash if this is actually called.
			*/
			assert(0 && "_impcheck called.");

			GetSymbol(NULL, NULL);
			CompileProject(NULL, NULL, NULL);
			SetErrorFunc(NULL, NULL, NULL);
			ReleaseProject(NULL);
			InitProject(NULL);
			ActivateProject(NULL);
			DisableProject(NULL, 0);
			GetState(NULL);
			AddSymbol(NULL, NULL, NULL);
			ProcessReplacing(NULL, NULL, NULL, 0);
			OnEvent(NULL, NULL);
		}

	#endif
#endif