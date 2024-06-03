
#include <inttypes.h>
#include <array>
#include <boost/hana.hpp>

template <uint32_t size, uint32_t init_ctr, typename CTR_TYPE, uint32_t ...SharedBits>
struct pht {

    constexpr static CTR_TYPE dummy_counter = CTR_TYPE();
    constexpr static uint8_t ctr_width = dummy_counter.width;

    static_assert(ctr_width == sizeof...(SharedBits), "Wrong number of shared bits!");

    consteval static auto init_arrays(){
        boost::hana::tuple<std::array<uint64_t, (size / 64) / SharedBits>...> tmp;

        uint32_t i = ctr_width - 1;
        boost::hana::for_each(tmp, [&](auto& elem) {
            uint64_t init_val;
            if((init_ctr >> i) & 1){
                init_val = 0xFFFFFFFFFFFFFFFF;
            } else {
                init_val = 0;
            }
            elem.fill(init_val);
            i--;
        });

        return tmp;
    };

    inline constinit static boost::hana::tuple<std::array<uint64_t, (size / 64) / SharedBits>...> arrays = init_arrays();
    std::array<uint32_t, sizeof...(SharedBits)> tmp_arr {SharedBits...};

    constexpr CTR_TYPE get_counter(uint32_t i){
        std::array<uint32_t, ctr_width> array;

        uint32_t iter = 0;
        boost::hana::for_each(arrays, [&](auto& elem) {
            uint32_t index = i >> (tmp_arr[iter] - 1);
            uint32_t arr_index = index >> 6;
            uint32_t line_index = index & 0x3F;

            uint64_t line = elem[arr_index];
            array[iter] = (line >> line_index) & 1;
            iter++;
        });

        return CTR_TYPE(array);
    }

    constexpr void save_counter(uint32_t i, CTR_TYPE counter){

        std::array array = counter.getArray();

        uint32_t iter = 0;
        boost::hana::for_each(arrays, [&](auto& elem) {
            uint32_t index = i >> (tmp_arr[iter] - 1);
            uint32_t arr_index = index >> 6;
            uint32_t line_index = index & 0x3F;

            uint64_t line = elem[arr_index];
            uint64_t MaskedLine = line & ~((uint64_t)1 << line_index);
            uint64_t NewLine = MaskedLine | ((uint64_t)array[iter] << line_index);
            elem[arr_index] = NewLine;
            iter++;
        });
    }

};

template <uint32_t CTR_WIDTH>
struct sat_ctr {
    uint32_t Dir;
    uint32_t Strenght;

    constexpr static uint32_t width = CTR_WIDTH;
    constexpr static uint32_t MaxStrenght = [](){
        uint32_t strenght = 0;
        for(uint32_t i = 0; i < CTR_WIDTH - 1; i++){
            strenght = strenght << 1;
            strenght |= 1;
        }
        return strenght;
    }();

    constexpr sat_ctr(std::array<uint32_t, CTR_WIDTH> arr){
        Dir = arr[0];
        Strenght = 0;

        for(uint32_t i = 1; i  < arr.size(); i++){
            Strenght |= (arr[i] << (i - 1));
        }

    }

    constexpr sat_ctr() : Dir(0), Strenght(0) {}

    constexpr std::array<uint32_t, CTR_WIDTH> getArray(){
        std::array<uint32_t, CTR_WIDTH> arr;

        arr[0] = Dir;
        for(uint32_t i = 1; i < arr.size(); i++){
            arr[i] = (Strenght >> (i - 1)) & 1;
        }

        return arr;

    }

    sat_ctr updateCounter(uint32_t updateDir){

        //__builtin_assume(Strenght <= MaxStrenght);
        //__builtin_assume(Dir < 2);
        //__builtin_assume(updateDir < 2);

        sat_ctr counter;
        counter.Dir = Dir;
        counter.Strenght = Strenght;

        if(updateDir == Dir){
            if(Strenght != MaxStrenght){
                counter.Strenght++;
            }
        } else {
            if(Strenght == 0){
                counter.Dir = Dir ^ 1;
            }
            else{
                counter.Strenght--;
            }
        }

        return counter;
    }

    uint32_t rawVal(){
        assert(Strenght <= MaxStrenght);
        assert(Dir < 2);
        return (Dir << (CTR_WIDTH - 1)) | Strenght;
    }
};

