PHP_ARG_ENABLE(undo,
  [Whether to enable the "undo" extension],
  [  enable-undo        Enable "undo" extension support])

if test $PHP_UNDO != "no"; then
  PHP_SUBST(UNDO_SHARED_LIBADD)
  PHP_NEW_EXTENSION(undo, undo.c ../undocall.c, $ext_shared)
fi
