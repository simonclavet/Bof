
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// sorry.. can't deal with std:: for such basic things, but 'using namespace std' is not a good idea.



#define SAFE_DELETE(x) if (x) {delete x; x = nullptr;}

