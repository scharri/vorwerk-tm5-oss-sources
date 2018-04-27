#ifndef __MXS_ROTARY_ENCODER_H__
#define __MXS_ROTARY_ENCODER_H__

struct mxs_rotary_encoder_platform_data {
	unsigned int steps;
	unsigned int axis;
	unsigned int gpio_a;
	unsigned int gpio_b;
	unsigned int gpio_push;
        unsigned int pin_idx_a;
        unsigned int pin_idx_b;
	unsigned int inverted_a;
	unsigned int inverted_b;
	bool relative_axis;
	bool rollover;
        void __iomem *base;
};

#endif /* __MXS_ROTARY_ENCODER_H__ */
