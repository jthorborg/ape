#ifndef LIBCPPJIT_H
#define LIBCPPJIT_H
#ifdef libCppJit_EXPORTS
#define LIBCPPJIT_EXPORT extern "C" __declspec(dllexport) 
#else
#define LIBCPPJIT_EXPORT extern "C" __declspec(dllimport) 
#endif

typedef struct JitContext jit_context;
typedef struct TranslationUnit translation_unit;

typedef enum
{
	jit_error_none,
	jit_error_unknown,
	jit_error_argument,
	jit_error_not_found,
	jit_error_unresolved_external,
	jit_error_unfit_stage,
	jit_error_at_previous_stage,
	jit_error_verbose,
	jit_error_parsing,
	jit_error_compilation_remark,
	jit_error_compilation_warning,
	jit_error_compilation_error

} jit_error_t;

typedef void(*jit_error_callback)(void * ctx, const char * message, jit_error_t error_type);

struct trs_unit_options
{
	size_t size;
	int argc;
	const char* const* argv;
	jit_error_callback callback;
	void* opaque;
	const char* name;
};

LIBCPPJIT_EXPORT jit_error_t trs_unit_from_string(const char* contents, trs_unit_options* options, translation_unit ** result);
LIBCPPJIT_EXPORT jit_error_t trs_unit_from_file(const char* code_file, const trs_unit_options* options, translation_unit** result);
LIBCPPJIT_EXPORT jit_error_t trs_unit_load_saved(const char* where, translation_unit** result);
LIBCPPJIT_EXPORT jit_error_t trs_unit_save(translation_unit * unit, const char* where);

LIBCPPJIT_EXPORT jit_error_t trs_unit_delete(translation_unit * unit);

LIBCPPJIT_EXPORT const char * jit_format_error(jit_error_t error);
LIBCPPJIT_EXPORT jit_error_t jit_create_context(jit_context** result);
LIBCPPJIT_EXPORT jit_error_t jit_add_translation_unit(jit_context * c, const translation_unit* unit);
LIBCPPJIT_EXPORT jit_error_t jit_set_callback(jit_context * c, void * callback_context, jit_error_callback cb);
LIBCPPJIT_EXPORT jit_error_t jit_delete_context(jit_context * c);
LIBCPPJIT_EXPORT jit_error_t jit_get_symbol(jit_context * c, const char * name, void ** ret);
LIBCPPJIT_EXPORT jit_error_t jit_inject_symbol(jit_context * c, const char * name, void * location);
LIBCPPJIT_EXPORT jit_error_t jit_finalize(jit_context * c);
LIBCPPJIT_EXPORT jit_error_t jit_open(jit_context * c);
LIBCPPJIT_EXPORT jit_error_t jit_close(jit_context * c);

#endif