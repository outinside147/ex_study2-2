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


// boxがboxesの中にあればtrue、そうでなければfalseを返す
bool set_member(BOX* box, Boxa* boxes){
	int i = 0;
	BOX* include_box = boxaGetBox(boxes, 0, L_CLONE);
	while (include_box != box && i != boxes->n - 1){
		i++;
		include_box = boxaGetBox(boxes, i, L_CLONE);
	}

	if (include_box == box){
		return true;
	}
	else {
		return false;
	}
}

// a_boxesの要素を一つずつ調べ、b_boxesの要素でないなら、c_boxesに加える
void set_difference(Boxa* a_boxes, Boxa* b_boxes, Boxa* c_boxes){
	int i, j;
	for (i = 0, j = 0; i < a_boxes->n; i++){
		BOX* include_abox = boxaGetBox(a_boxes, i, L_CLONE);
		if (!set_member(include_abox, b_boxes)){
			boxaAddBox(c_boxes, include_abox, L_CLONE);
		}
	}
}

int main()
{
	ofstream content("../image/component.txt");
	ofstream sortx("../image/sorted_x.txt");
	ofstream sorty("../image/sorted_y.txt");

	Mat src = imread("../../../source_images/img1_3_2.jpg", 1);
	Pix *image = pixRead("../../../source_images/img1_3_2.jpg");
	Mat para_map = src.clone();
	Mat word_map1 = src.clone();
	Mat word_map2 = src.clone();

	int lparam_end = 0;
	int rparam_end = 0;

	TessBaseAPI *api = new TessBaseAPI();
	api->Init(NULL, "eng");
	api->SetImage(image);
	Boxa* para_boxes = api->GetComponentImages(RIL_PARA, true, NULL, NULL); //段落
	Boxa* word_boxes = api->GetComponentImages(RIL_WORD, true, NULL, NULL); //単語

	for (int i = 0; i < para_boxes->n; i++) {
		BOX* box = boxaGetBox(para_boxes, i, L_CLONE); //boxes->n .. number of box in ptr array
		lparam_end = box->x; // 段落の左端
		rparam_end = box->x + box->w; //段落の右端
		api->SetRectangle(box->x, box->y, box->w, box->h);
		// 画像に分割した範囲を描写する
		rectangle(para_map, Point(box->x, box->y), Point(box->x+box->w, box->y+box->h),Scalar(0,0,255),1,4);
		imwrite("../image/splitImages/map_para.png", para_map);
	}

	// 明らかに誤認識している単語を消去する
	Boxa* valid_boxes = boxaCreate(word_boxes->n);
	for (int i = 0; i < word_boxes->n; i++) {
		BOX* box = boxaGetBox(word_boxes, i, L_CLONE);
		if (box->h > 3 && box->w > 3){ //画像を変更したので、閾値を調整**
			boxaAddBox(valid_boxes, box, L_CLONE);
		}
	}

	// 単語単位での認識結果を書き出す
	for (int i = 0; i < valid_boxes->n; i++) {
		BOX* box = boxaGetBox(valid_boxes, i, L_CLONE);
		api->SetRectangle(box->x, box->y, box->w, box->h);
		char* ocrResult = api->GetUTF8Text();
		int conf = api->MeanTextConf();
		content << "Word_Box[" << i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << ", confidence=" << conf << ", text= " << ocrResult << endl;
		
		
		// 分割した範囲を切り取ってそれぞれの画像にする
		Rect rect(box->x, box->y, box->w, box->h);
		Mat part_img(src, rect);
		imwrite("../image/splitImages/word_" + to_string(i) + ".png", part_img);
		rectangle(word_map1, Point(box->x, box->y), Point(box->x + box->w, box->y + box->h), Scalar(255, 0, 0), 1, 4);
		imwrite("../image/splitImages/map_word_valid.png", word_map1);
		
	}

	Boxa* sort_xboxes = boxaCreate(500);
	Boxa* sort_yboxes = boxaCreate(100);
	Boxa* leading_boxes = boxaCreate(100);

	// xの昇順でソート
	sort_xboxes = boxaSort(valid_boxes, L_SORT_BY_X, L_SORT_INCREASING,NULL);
	for (int i = 0; i < sort_xboxes->n; i++){
		BOX* box = boxaGetBox(sort_xboxes, i, L_CLONE);
		rectangle(word_map1, Point(box->x, box->y), Point(box->x + box->w, box->y + box->h), Scalar(255, 0, 255), 1, 4);
		imwrite("../image/splitImages/map_word_xsort.png", word_map1);
		sortx << "sorted_x[" << i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << endl;
	}

	// ソートした配列の先頭から100個の単語を行の先頭候補として別の配列に格納する
	for (int i = 0; i < valid_boxes->n/10; i++){
		BOX* box = boxaGetBox(sort_xboxes, i, L_CLONE);
		boxaAddBox(leading_boxes, box, L_CLONE);
	}
	
	// yの昇順でソート
	sort_yboxes = boxaSort(leading_boxes, L_SORT_BY_Y, L_SORT_INCREASING, NULL);
	for (int i = 0; i < sort_yboxes->n; i++){
		BOX* box = boxaGetBox(sort_yboxes, i, L_CLONE);
		rectangle(word_map2, Point(box->x, box->y), Point(box->x + box->w, box->y + box->h), Scalar(255, 255, 0), 1, 4);
		imwrite("../image/splitImages/map_word_ysort.png", word_map2);
		sorty << "sorted_y[" << i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << endl;
	}


}