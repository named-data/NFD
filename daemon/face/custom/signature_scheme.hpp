#include <vector>
#include <cstdint>
#include <string>

class signature_scheme{

    protected:
    std::vector<std::uint8_t> id; 
    public:
    virtual void setup()=0;
    virtual void key_derivation()=0;
    virtual void sign()=0;
    virtual bool verify()=0;

};