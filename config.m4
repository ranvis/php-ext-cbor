dnl config.m4 for extension cbor

PHP_ARG_WITH(cbor, for cbor support,
[  --with-cbor             Include cbor support])

if test "$PHP_CBOR" != "no"; then

  dnl # --with-cbor -> check with-path
  SEARCH_PATH="/usr/local /usr"
  SEARCH_FOR="/include/cbor.h"
  if test -r $PHP_CBOR/$SEARCH_FOR; then # path given as parameter
    CBOR_DIR=$PHP_CBOR
  else # search default path list
    AC_MSG_CHECKING([for cbor files in default path])
    for i in $SEARCH_PATH ; do
      if test -r $i/$SEARCH_FOR; then
        CBOR_DIR=$i
        AC_MSG_RESULT(found in $i)
      fi
    done
  fi
  
  if test -z "$CBOR_DIR"; then
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([Please reinstall the libcbor distribution])
  fi

  # --with-cbor -> add include path
  PHP_ADD_INCLUDE($CBOR_DIR/include)

  # --with-cbor -> check for lib and symbol presence
  LIBNAME=cbor
  LIBSYMBOL=cbor_typeof

  PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  [
    PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $CBOR_DIR/$PHP_LIBDIR, CBOR_SHARED_LIBADD)
    AC_DEFINE(HAVE_CBORLIB,1,[ ])
  ],[
    AC_MSG_ERROR([wrong libcbor version or lib not found])
  ],[
    -L$CBOR_DIR/$PHP_LIBDIR -lm
  ])
  
  PHP_SUBST(CBOR_SHARED_LIBADD)

  dnl libcbor uses C99 restrict keyword
  PHP_NEW_EXTENSION(cbor, src/cbor.c src/compatibility.c src/decode.c src/encode.c src/functions.c src/options.c src/types.c src/utf8.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1 -std=c99)
fi
