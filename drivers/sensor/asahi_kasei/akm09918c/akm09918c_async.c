/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "akm09918c.h"

LOG_MODULE_DECLARE(AKM09918C, CONFIG_SENSOR_LOG_LEVEL);

void akm09918_async_fetch(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct akm09918c_async_fetch_ctx *ctx =
		CONTAINER_OF(dwork, struct akm09918c_async_fetch_ctx, async_fetch_work);

	uint32_t req_buf_len = sizeof(struct akm09918c_encoded_data);
	uint32_t buf_len;
	uint8_t *buf;
	struct akm09918c_encoded_data *edata;
	int rc;

	/* Get the buffer for the frame, it may be allocated dynamically by the rtio context */
	rc = rtio_sqe_rx_buf(ctx->iodev_sqe, req_buf_len, req_buf_len, &buf, &buf_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", req_buf_len);
		rtio_iodev_sqe_err(ctx->iodev_sqe, rc);
		return;
	}
	edata = (struct akm09918c_encoded_data *)buf;
	rc = akm09918c_fetch_measurement(ctx->dev, &edata->readings[0], &edata->readings[1],
					 &edata->readings[2]);
	if (rc != 0) {
		rtio_iodev_sqe_err(ctx->iodev_sqe, rc);
		return;
	}
	rtio_iodev_sqe_ok(ctx->iodev_sqe, 0);
}

int akm09918c_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	int rc;
	struct akm09918c_data *data = dev->data;

	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;

	/* check for invalid channel count (only 1 channel is supported yet) */
	if (cfg->count != 1) {
		LOG_ERR("More than one channel is not yet supported by the driver.");
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}

	/* start the measurement in the sensor */
	rc = akm09918c_start_measurement(dev, cfg->channels[0].chan_type);
	if (rc != 0) {
		LOG_ERR("Failed to fetch samples.");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	/* save information for the work item */
	data->work_ctx.timestamp = k_ticks_to_ns_floor64(k_uptime_ticks());
	data->work_ctx.iodev_sqe = iodev_sqe;

	rc = k_work_schedule(&data->work_ctx.async_fetch_work, K_USEC(AKM09918C_MEASURE_TIME_US));
	if (rc == 0) {
		LOG_ERR("The last fetch has not finished yet. "
			"Try again later when the last sensor read operation has finished.");
		rtio_iodev_sqe_err(iodev_sqe, -EBUSY);
		return;
	}
	return 0;
}
