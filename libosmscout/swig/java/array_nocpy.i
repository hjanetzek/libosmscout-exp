%{
/* float[] support */
static int SWIG_JavaArrayInFloat (JNIEnv *jenv, jfloat **jarr, float **carr, jfloatArray input) {
  if (!input) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null array");
    return 0;
  }

   jboolean isCopy;

   *carr = (float*) jenv->GetPrimitiveArrayCritical(input, &isCopy);
   if (!*carr)
      return 0;

  return 1;
}

static void SWIG_JavaArrayArgoutFloat (JNIEnv *jenv, jfloat *jarr, float *carr, jfloatArray input) {
  /* int i;
   * jsize sz = jenv->GetArrayLength(input);
   * for (i=0; i<sz; i++)
   *   jarr[i] = (jfloat)carr[i];
   * jenv->ReleaseFloatArrayElements(input, jarr, 0); */

  jenv->ReleasePrimitiveArrayCritical(input, carr, JNI_ABORT);
}

static jfloatArray SWIG_JavaArrayOutFloat (JNIEnv *jenv, float *result, jsize sz) {
  jfloat *arr;
  int i;
  jfloatArray jresult = jenv->NewFloatArray(sz);
  if (!jresult)
    return NULL;
  arr = jenv->GetFloatArrayElements(jresult, 0);
  if (!arr)
    return NULL;
  for (i=0; i<sz; i++)
    arr[i] = (jfloat)result[i];
  jenv->ReleaseFloatArrayElements(jresult, arr, 0);
  return jresult;
}
%}
/*@SWIG@*/         /* float[] */
/*@SWIG:arrays_java.i,38,JAVA_ARRAYS_IMPL@*/

%{
static int SWIG_JavaArrayInFloat (JNIEnv *jenv, jfloat **jarr, float **carr, jfloatArray input);
static void SWIG_JavaArrayArgoutFloat (JNIEnv *jenv, jfloat *jarr, float *carr, jfloatArray input);
static jfloatArray SWIG_JavaArrayOutFloat (JNIEnv *jenv, float *result, jsize sz);
%}
/*@SWIG:arrays_java.i,29,JAVA_ARRAYS_DECL@*/


%typemap(jni) float[ANY], float[]               %{jfloatArray%}
%typemap(jtype) float[ANY], float[]             %{float[]%}
%typemap(jstype) float[ANY], float[]            %{float[]%}

%typemap(in) float[] (jfloat *jarr)
%{  if (!SWIG_JavaArrayInFloat(jenv, &jarr, (float **)&$1, $input)) return $null; %}

%typemap(in) float[ANY] (jfloat *jarr)
%{  if ($input && jenv->GetArrayLength($input) != $1_size) {
    SWIG_JavaThrowException(jenv, SWIG_JavaIndexOutOfBoundsException, "incorrect array size");
    return $null;
  }
  if (!SWIG_JavaArrayInFloat(jenv, &jarr, (float **)&$1, $input)) return $null; 
%}

%typemap(argout) float[ANY], float[] 
%{ SWIG_JavaArrayArgoutFloat(jenv, jarr$argnum, (float *)$1, $input); %}

%typemap(out) float[ANY]
%{$result = SWIG_JavaArrayOutFloat(jenv, (float *)$1, $1_dim0); %}

%typemap(out) float[] 
%{$result = SWIG_JavaArrayOutFloat(jenv, (float *)$1, FillMeInAsSizeCannotBeDeterminedAutomatically); %}

/* %typemap(freearg) float[ANY], float[] 
 * %{ delete [] $1; %} */

%typemap(javain) float[ANY], float[] "$javainput"

%typemap(javaout) float[ANY], float[] {
    return $jnicall;
  }

%typemap(memberin) float[ANY], float[];
%typemap(globalin) float[ANY], float[];
/*@SWIG@*/         /* float[ANY] */
/*@SWIG:arrays_java.i,143,JAVA_ARRAYS_TYPEMAPS@*/


%typemap(typecheck, precedence=      1080    )  /* Java float[] */
    float[ANY], float[]
    ""


