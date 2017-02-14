#ifndef _CGW_DIPI_H
#define _CGW_DIPI_H

#define TYPE_PI                  0x1
#define TYPE_DI                  0x2

//gpio input
#define ON_CYCLE                  0x00
#define OFF_CYCLE                 0x01

// one cycle-> state ON + state OFF
#define STATE_NULL                0x00 //initial state
#define STATE_ON                  0x01
#define STATE_OFF                 0x02
#define STATE_ON_FINISH           0x04
#define STATE_OFF_FINISH          0x08
#define STATE_MASK                0x03
#define STATE_FINISH_MASK         0x0c

#define en_to_dipi_port(_dev_attr)                              \
	container_of(_dev_attr, struct __dipi_port_data, en_attr)

#define cnt_to_dipi_port(_dev_attr)                              \
	container_of(_dev_attr, struct __dipi_port_data, cnt_attr)

#define name_en(X) X##_EN
#define name_cnt(X) X##_CNT

#define DIPI_POART_ATTR(_name, _mode)     \
		 struct __dipi_port_data port_##_name                \
			= { .name = __stringify(_name),                     \
				.en_attr = __ATTR(name_en(_name), _mode, DIPI_EN_attr_show, DIPI_EN_attr_store),  \
			     .cnt_attr = __ATTR(name_cnt(_name), _mode, DIPI_CNT_attr_show, DIPI_CNT_attr_store),  \
			}

static ssize_t DIPI_EN_attr_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t len);

static ssize_t DIPI_EN_attr_show(struct device *dev,
					struct device_attribute *attr,
					char *buf);

static ssize_t DIPI_CNT_attr_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t len);

static ssize_t DIPI_CNT_attr_show(struct device *dev,
					struct device_attribute *attr,
					char *buf);
#endif

