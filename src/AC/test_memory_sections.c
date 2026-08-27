/* Test memory section keywords */

__chip int chip_data = 42;
__far int far_data = 100;
__near int near_data = 200;
__fast int fast_data = 300;

int main() {
    return chip_data + far_data + near_data + fast_data;
}
