#include "undo.h"
#include "../undocall.h"

PHP_FUNCTION(undo_mask_start)
{
	zval *arg1;
	php_stream *stream;
	int fd;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	PHP_STREAM_TO_ZVAL(stream, &arg1);
	if (php_stream_cast(stream, PHP_STREAM_AS_FD, (void **)&fd, REPORT_ERRORS) == FAILURE) {
		RETURN_FALSE;
	}

	undo_mask_start(fd);
	RETURN_TRUE;
}

PHP_FUNCTION(undo_mask_end)
{
	zval *arg1;
	php_stream *stream;
	int fd;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	PHP_STREAM_TO_ZVAL(stream, &arg1);
	if (php_stream_cast(stream, PHP_STREAM_AS_FD, (void **)&fd, REPORT_ERRORS) == FAILURE) {
		RETURN_FALSE;
	}

	undo_mask_end(fd);
	RETURN_TRUE;
}

PHP_FUNCTION(undo_func_start)
{
	char *arg1, *arg2, *arg3;
	int arg1len, arg2len, arg3len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sss", &arg1, &arg1len, &arg2, &arg2len, &arg3, &arg3len) == FAILURE) {
		RETURN_FALSE;
	}

	undo_func_start(arg1, arg2, arg3len, arg3);
	RETURN_TRUE;
}

PHP_FUNCTION(undo_func_end)
{
	char *arg1;
	int arg1len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg1, &arg1len) == FAILURE) {
		RETURN_FALSE;
	}

	undo_func_end(arg1len, arg1);
	RETURN_TRUE;
}

PHP_FUNCTION(undo_depend)
{
	zval *arg1;
	php_stream *stream;
	int fd;
	char *arg2, *arg3;
	int arg2len, arg3len;
	zend_bool arg4, arg5;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rssbb", &arg1, &arg2, &arg2len, &arg3, &arg3len, &arg4, &arg5) == FAILURE) {
		RETURN_FALSE;
	}

	PHP_STREAM_TO_ZVAL(stream, &arg1);
	if (php_stream_cast(stream, PHP_STREAM_AS_FD, (void **)&fd, REPORT_ERRORS) == FAILURE) {
		RETURN_FALSE;
	}

	undo_depend(fd, arg2, arg3, (arg4)?1:0, (arg5)?1:0);
	RETURN_TRUE;
}

PHP_FUNCTION(undo_depend_to)
{
	zval *arg1;
	php_stream *stream;
	int fd;
	char *arg2, *arg3;
	int arg2len, arg3len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &arg1, &arg2, &arg2len, &arg3, &arg3len) == FAILURE) {
		RETURN_FALSE;
	}

	PHP_STREAM_TO_ZVAL(stream, &arg1);
	if (php_stream_cast(stream, PHP_STREAM_AS_FD, (void **)&fd, REPORT_ERRORS) == FAILURE) {
		RETURN_FALSE;
	}

	undo_depend_to(fd, arg2, arg3);
	RETURN_TRUE;
}

PHP_FUNCTION(undo_depend_from)
{
	zval *arg1;
	php_stream *stream;
	int fd;
	char *arg2, *arg3;
	int arg2len, arg3len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &arg1, &arg2, &arg2len, &arg3, &arg3len) == FAILURE) {
		RETURN_FALSE;
	}

	PHP_STREAM_TO_ZVAL(stream, &arg1);
	if (php_stream_cast(stream, PHP_STREAM_AS_FD, (void **)&fd, REPORT_ERRORS) == FAILURE) {
		RETURN_FALSE;
	}

	undo_depend_from(fd, arg2, arg3);
	RETURN_TRUE;
}

PHP_FUNCTION(undo_depend_both)
{
	zval *arg1;
	php_stream *stream;
	int fd;
	char *arg2, *arg3;
	int arg2len, arg3len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &arg1, &arg2, &arg2len, &arg3, &arg3len) == FAILURE) {
		RETURN_FALSE;
	}

	PHP_STREAM_TO_ZVAL(stream, &arg1);
	if (php_stream_cast(stream, PHP_STREAM_AS_FD, (void **)&fd, REPORT_ERRORS) == FAILURE) {
		RETURN_FALSE;
	}

	undo_depend_both(fd, arg2, arg3);
	RETURN_TRUE;
}

static function_entry php_undo_functions[] = {
	PHP_FE(undo_mask_start, NULL)
	PHP_FE(undo_mask_end, NULL)
	PHP_FE(undo_func_start, NULL)
	PHP_FE(undo_func_end, NULL)
	PHP_FE(undo_depend, NULL)
	PHP_FE(undo_depend_to, NULL)
	PHP_FE(undo_depend_from, NULL)
	PHP_FE(undo_depend_both, NULL)
	NULL,
};

zend_module_entry undo_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	PHP_UNDO_EXTNAME,
	php_undo_functions, /* Functions */
	NULL, /* MINIT */
	NULL, /* MSHUTDOWN */
	NULL, /* RINIT */
	NULL, /* RSHUTDOWN */
	NULL, /* MINFO */
#if ZEND_MODULE_API_NO >= 20010901
	PHP_UNDO_EXTVER,
#endif
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_UNDO
ZEND_GET_MODULE(undo)
#endif
