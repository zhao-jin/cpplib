/* -----------------------------------------------------------------------------
 * binary_string.i
 *
 * Typemaps for binary_string and const binary_string&
 * These are mapped to a Java String and are passed around by value.
 *
 * To use non-const binary_string references use the following %apply.  Note
 * that they are passed by value.
 * %apply const binary_string & { binary_string & };
 * ----------------------------------------------------------------------------- */

%{
#include <string>
%}

// binary_string
%typemap(jni) binary_string "jbyteArray"
%typemap(jtype) binary_string "byte[]"
%typemap(jstype) binary_string "byte[]"
%typemap(javadirectorin) binary_string "$jniinput"
%typemap(javadirectorout) binary_string "$javacall"

%typemap(in) binary_string
%{ if(!$input) {
     SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null std::string");
     return $null;
    }
    const char *$1_pstr = (const char *)jenv->GetByteArrayElements($input, 0);
    if (!$1_pstr) return $null;
    size_t $1_len = jenv->GetArrayLength($input);
    $1.assign($1_pstr, $1_len);
    jenv->ReleaseByteArrayElements($input, (jbyte*)$1_pstr, 0); %}

%typemap(directorout) binary_string
%{ if(!$input) {
     SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null std::string");
     return $null;
    }
    const char *$1_pstr = (const char *)jenv->GetByteArrayElements($input, 0);
    if (!$1_pstr) return $null;
    size_t $1_len = jenv->GetArrayLength($input);
    $1.assign($1_pstr, $1_len);
    jenv->ReleaseByteArrayElements($input, (jbyte*)$1_pstr, 0); %}

%typemap(directorin,descriptor="[B") binary_string
%{  jsize $1_len = (jsize)$1.size();
    jbyteArray $1_jb = jenv->NewByteArray($1_len);
    jenv->SetByteArrayRegion($1_jb, 0, $1_len, (jbyte*)$1.c_str());
    $input = $1_jb; %}

%typemap(out) binary_string
%{  jsize $1_len = (jsize)$1.size();
    jbyteArray $1_jb = jenv->NewByteArray($1_len);
    jenv->SetByteArrayRegion($1_jb, 0, $1_len, (jbyte*)$1.c_str());
    $result = $1_jb; %}

%typemap(javain) binary_string "$javainput"

%typemap(javaout) binary_string {
    return $jnicall;
  }

%typemap(typecheck) binary_string = char *;

%typemap(throws) binary_string
%{ SWIG_JavaThrowException(jenv, SWIG_JavaRuntimeException, $1.c_str());
   return $null; %}

// const binary_string &
%typemap(jni) const binary_string & "jbyteArray"
%typemap(jtype) const binary_string & "byte[]"
%typemap(jstype) const binary_string & "byte[]"
%typemap(javadirectorin) const binary_string & "$jniinput"
%typemap(javadirectorout) const binary_string & "$javacall"

%typemap(in) const binary_string &
%{ if(!$input) {
     SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null std::string");
     return $null;
   }
   const char *$1_pstr = (const char *)jenv->GetByteArrayElements($input, 0);
   if (!$1_pstr) return $null;
   size_t $1_len = jenv->GetArrayLength($input);
   std::string $1_str($1_pstr, $1_len);
   $1 = &$1_str;
   jenv->ReleaseByteArrayElements($input, (jbyte*)$1_pstr, 0); %}

%typemap(directorout,warning=SWIGWARN_TYPEMAP_THREAD_UNSAFE_MSG) const binary_string &
%{ if(!$input) {
     SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null std::string");
     return $null;
   }
   const char *$1_pstr = (const char *)jenv->GetByteArrayElements($input, 0);
   if (!$1_pstr) return $null;
   size_t $1_len = jenv->GetArrayLength($input);
   std::string $1_str($1_pstr, $1_len);
   /* possible thread/reentrant code problem */
   static std::string $1_str;
   $1_str.swap(std::string($1_pstr, $1_len));
   $result = &$1_str;
   jenv->ReleaseByteArrayElements($input, (jbyte*)$1_pstr, 0); %}

%typemap(directorin,descriptor="[B") const binary_string &
%{  jsize $1_len = (jsize)$1.size();
    jbyteArray $1_jb = jenv->NewByteArray($1_len);
    jenv->SetByteArrayRegion($1_jb, 0, $1_len, (jbyte*)$1.c_str());
    $input = $1_jb; %}

%typemap(out) const binary_string &
%{  jsize $1_len = (jsize)$1.size();
    jbyteArray $1_jb = jenv->NewByteArray($1_len);
    jenv->SetByteArrayRegion($1_jb, 0, $1_len, (jbyte*)$1.c_str());
    $result = $1_jb; %}

%typemap(javain) const binary_string & "$javainput"

%typemap(javaout) const binary_string & {
    return $jnicall;
  }

%typemap(typecheck) const binary_string & = char *;

%typemap(throws) const binary_string &
%{ SWIG_JavaThrowException(jenv, SWIG_JavaRuntimeException, $1.c_str());
   return $null; %}

