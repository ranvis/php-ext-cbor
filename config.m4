dnl $Id$
dnl config.m4 for extension cbor

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(cbor, for cbor support,
dnl Make sure that the comment is aligned:
dnl [  --with-cbor             Include cbor support])

dnl Otherwise use enable:

dnl PHP_ARG_ENABLE(cbor, whether to enable cbor support,
dnl Make sure that the comment is aligned:
dnl [  --enable-cbor           Enable cbor support])

if test "$PHP_CBOR" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-cbor -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/cbor.h"  # you most likely want to change this
  dnl if test -r $PHP_CBOR/$SEARCH_FOR; then # path given as parameter
  dnl   CBOR_DIR=$PHP_CBOR
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for cbor files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       CBOR_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$CBOR_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the cbor distribution])
  dnl fi

  dnl # --with-cbor -> add include path
  dnl PHP_ADD_INCLUDE($CBOR_DIR/include)

  dnl # --with-cbor -> check for lib and symbol presence
  dnl LIBNAME=cbor # you may want to change this
  dnl LIBSYMBOL=cbor # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $CBOR_DIR/$PHP_LIBDIR, CBOR_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_CBORLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong cbor lib version or lib not found])
  dnl ],[
  dnl   -L$CBOR_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  dnl PHP_SUBST(CBOR_SHARED_LIBADD)

  PHP_NEW_EXTENSION(cbor, cbor.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
fi
