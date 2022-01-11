
g++ -std=c++20 -E -P -C -xc++ - <<__eos__

#define MINSL_EXECUTION_CHECK(condition, place, errorMessage) \
  if(!(condition)) \
  { \
    throw ExecutionError((place), (errorMessage)); \
  }

$(cat)
__eos__

