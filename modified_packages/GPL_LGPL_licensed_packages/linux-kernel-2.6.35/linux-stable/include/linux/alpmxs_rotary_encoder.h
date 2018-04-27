#ifndef __ROTARY_ENCODER_H__
#define __ROTARY_ENCODER_H__

struct alpmxs_rotary_encoder_platform_data {
	unsigned int steps;
	unsigned int axis;
	unsigned int gpio_a1;
	unsigned int gpio_b1;
	unsigned int gpio_a2;
	unsigned int gpio_b2;
	unsigned int gpio_push;
	unsigned int inverted_a;
	unsigned int inverted_b;
        unsigned int inverted_push;
	bool relative_axis;
	bool rollover;
        void __iomem *base;
};

#endif /* __ROTARY_ENCODER_H__ */
