char *base64_encode(const unsigned char *data,
                    size_t input_length,
                    size_t *output_length) ;

void send_base64_encode(const unsigned char *data,
                    size_t input_length) ;
                    
unsigned char *base64_decode(const char *data,
                             size_t input_length,
                             size_t *output_length);
void test_b64encodedecode(void);


