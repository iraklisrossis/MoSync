/*
 * BarcodeScanner.h
 *
 *  Created on: Feb 28, 2012
 *      Author: Anders
 */

#ifndef BARCODESCANNER_H_
#define BARCODESCANNER_H_

#include <ma.h>
#include <string>
#include <zbar.h>

class BarcodeScanner
{
public:
	BarcodeScanner();
	virtual ~BarcodeScanner();

	void initiate();
	void shutdown();

	void uploadHandle(MAHandle image);
	void uploadRGB888(int* img, int width, int height);

	bool getBarcode(char* barcodeType, char* barcode);

private:

	void upload(int* img, int width, int height);
	const char* getBarcodeType(int type);

	zbar::zbar_image_t *mImage;

	unsigned char* mImgData;

	zbar::ImageScanner mImageScanner;

};


#endif /* BARCODESCANNER_H_ */
