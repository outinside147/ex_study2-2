
#define _CRT_SECURE_NO_WARNINGS

#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <tesseract/strngs.h>

#include <iostream>
#include <fstream>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

using namespace cv;
using namespace std;
using namespace tesseract;

int main()
{
	// 結果をテキストに出力
	ofstream ofs("../image/component.txt");
	Mat src = imread("../../../source_images/img1_3_2.jpg",1);
	Pix *image = pixRead("../../../source_images/img1_3_2.jpg");
	Mat para_map = src.clone();
	Mat line_map = src.clone();
	Mat word_map = src.clone();
	
	TessBaseAPI *api = new TessBaseAPI();
	api->Init(NULL, "eng");
	api->SetImage(image);
	Boxa* para_boxes = api->GetComponentImages(RIL_PARA, true, NULL, NULL); //段落
	Boxa* line_boxes = api->GetComponentImages(RIL_TEXTLINE, true, NULL, NULL); //行
	Boxa* word_boxes = api->GetComponentImages(RIL_WORD, true, NULL, NULL); //単語
	printf("Found textline = %d , word = %d .\n", line_boxes->n,word_boxes->n);
	ofs << "found textline = " << line_boxes->n << ",word = " << word_boxes->n << endl << endl;

	for (int i = 0; i < para_boxes->n; i++) {		
		BOX* box = boxaGetBox(para_boxes, i, L_CLONE); //boxes->n .. number of box in ptr array
		api->SetRectangle(box->x, box->y, box->w, box->h);

		// 認識結果をテキストファイルとして出力する
		ofs << "Para_Box["<< i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << endl;

		// 分割した範囲を切り取ってそれぞれの画像にする
		Rect rect(box->x, box->y, box->w, box->h);
		Mat para_img(src, rect);
		imwrite("../image/splitImages/para_" + to_string(i) + ".png", para_img);
		
		// 画像に分割した範囲を描写する
		rectangle(para_map, Point(box->x, box->y), Point(box->x+box->w, box->y+box->h),Scalar(0,0,255),1,4);
		imwrite("../image/splitImages/para_map.png", para_map);
	}

	for (int i = 0; i < line_boxes->n; i++) {
		BOX* box = boxaGetBox(line_boxes, i, L_CLONE);
		api->SetRectangle(box->x, box->y, box->w, box->h);
		char* ocrResult = api->GetUTF8Text();
		int conf = api->MeanTextConf();


		// 認識結果をテキストファイルとして出力する
		ofs << "Textline_Box[" << i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << ", confidence=" << conf << ", text=" << ocrResult << endl;

		// 分割した範囲を切り取ってそれぞれの画像にする
		Rect rect(box->x, box->y, box->w, box->h);
		Mat line_img(src, rect);
		imwrite("../image/splitImages/line_" + to_string(i) + ".png", line_img);

		// 画像に分割した範囲を描写する
		rectangle(line_map, Point(box->x, box->y), Point(box->x + box->w, box->y + box->h), Scalar(0, 255, 0), 1, 4);
		imwrite("../image/splitImages/line_map.png", line_map);
	}
	
	for (int i = 0; i < word_boxes->n; i++) {
		BOX* box = boxaGetBox(word_boxes, i, L_CLONE); //para_boxes->n .. number of box in ptr array
		api->SetRectangle(box->x, box->y, box->w, box->h);
		char* ocrResult = api->GetUTF8Text();
		int conf = api->MeanTextConf();

		// 認識結果をテキストファイルとして出力する
		ofs << "Word_Box[" << i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << ", confidence=" << conf << ", text=" << ocrResult << endl;

		// 分割した範囲を切り取ってそれぞれの画像にする
		Rect rect(box->x, box->y, box->w, box->h);
		Mat word_img(src, rect);
		imwrite("../image/splitImages/word_" + to_string(i) + ".png", word_img);

		// 画像に分割した範囲を描写する
		rectangle(word_map, Point(box->x, box->y), Point(box->x + box->w, box->y + box->h), Scalar(255, 0, 0), 1, 4);
		imwrite("../image/splitImages/word_map.png", word_map);
	}
}

