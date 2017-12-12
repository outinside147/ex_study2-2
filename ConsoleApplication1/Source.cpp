#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES

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

#define PI  3.14159265358979323846
#define MAX_X 9999.0

using namespace cv;
using namespace std;
using namespace tesseract;

// グローバル変数
Mat src = imread("../../../source_images/img1_3_2.jpg", 1); //入力画像
Pix *image = pixRead("../../../source_images/img1_3_2.jpg"); //入力画像(tesseract,leptonicaで使用する型)
Mat para_map = src.clone();
Mat mat_para_img; //抽出した段落画像

// BOX型の中心点を格納する構造体
typedef struct { double x, y;} Box_array;

// 結果を書きだす関数(map)
void outputPartImage(BOX* box, string file_name, Mat image, int i){
	Rect rect(box->x, box->y, box->w, box->h);
	Mat part_img(image, rect);
	imwrite(file_name + to_string(i) + ".png", part_img);
}

// boxがboxesの中にあればtrue、そうでなければfalseを返す
bool setMember(BOX* box, Boxa* boxes){
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

// a_boxesの要素を一つずつ調べ、b_boxesの要素でないなら、c_boxesに加える。これを返り値として返す
Boxa* setDiff(Boxa* a_boxes, Boxa* b_boxes){
	int i, j;
	Boxa* c_boxes = boxaCreate(a_boxes->n);
	for (i = 0, j = 0; i < a_boxes->n; i++){
		BOX* include_abox = boxaGetBox(a_boxes, i, L_CLONE);
		if (!setMember(include_abox, b_boxes)){
			boxaAddBox(c_boxes, include_abox, L_CLONE);
		}
	}
	return c_boxes;
}

// 単語の中心座標を求める
Box_array getCenterPoint(BOX* box){
	Box_array center_point = { 0, 0 };
	center_point.x = box->x + (box->w / 2);
	center_point.y = box->y + (box->h / 2);
	return center_point;
}

// 2つの単語の中心座標を取得し、そのベクトルを求める
Box_array getVector(BOX* box1, BOX* box2){
	Box_array vec = {0,0};
	Box_array st_center = getCenterPoint(box1);
	Box_array ed_center = getCenterPoint(box2);
	vec.x = ed_center.x - st_center.x;
	vec.y = ed_center.y - st_center.y;
	return vec;
}

// ベクトルの大きさを取得する。返り値は√(x^2 + y^2)
double getVectorLength(Box_array box){
	return sqrt((box.x * box.x) + (box.y * box.y));
}

// 2つの単語の内積を求める
double getProduct(Box_array box, Box_array base_box){
	return (box.x * base_box.x) + (box.y * base_box.y);
}

// BOXを作成し、それを返り値として返す
BOX* create_box(int x, int y, int w, int h){
	BOX* box = boxCreate(x, y, w, h);
	return box;
}

// 色の設定
Scalar setColor(int ci){
	Scalar color;
	switch (ci){
	case 0:
		color = Scalar(255, 0, 0); //ブルー
		break;
	case 1:
		color = Scalar(255, 255, 0); //シアン
		break;
	case 2:
		color = Scalar(0, 0, 255); //レッド
		break;
	case 3:
		color = Scalar(255, 0, 255); //マゼンタs
		break;
	}
	return color;
}

// 最頻値を求める , 入力=全単語列(valid_boxes) **編集
void findMode(Boxa* boxes){
	int class_num = ceil(1+log2((double)boxes->n)); //階級の数 スタージェスの公式: class_num = 1+log2n ceil=小数点の切り上げ
	Boxa* sort_asc = boxaSort(boxes, L_SORT_BY_HEIGHT, L_SORT_INCREASING, NULL); //昇順ソート
	Boxa* sort_desc = boxaSort(boxes, L_SORT_BY_HEIGHT, L_SORT_DECREASING, NULL); //降順ソート
	int min_h = boxaGetBox(sort_asc, 0, L_CLONE)->h; //高さの最低値を取得する
	int max_h = boxaGetBox(sort_desc, 0, L_CLONE)->h; //高さの最高値を取得する
	double dstrb = double(max_h - min_h) / (double)class_num; //分布の間隔

	printf("class_num=%3d, max_h=%3d, min_h=%3d, distribution=%1.3lf\n", class_num,max_h,min_h,dstrb);

	int rank = 0;
	vector<int> hist(class_num);
	for (int i = 0; i < boxes->n; i++){
		BOX* box = boxaGetBox(boxes, i, L_CLONE);
		rank = box->h / dstrb;
		printf("rank=%d\n", rank);
		if (0 <= rank && rank < class_num){
			hist[rank]++;
		}
	}

	for (int i = 0; i < class_num; i++){
		printf("\n%lf-%lf : %3d人", double((i*dstrb) + 0.1), double((i + 1)*dstrb), hist.at(i));
	}
	printf("\n");
}

int main()
{	
	ofstream content("../image/component.txt");
	
	TessBaseAPI *api = new TessBaseAPI();
	api->Init(NULL, "eng");
	api->SetImage(image);
	//画像中の段落を抽出する
	Boxa* para_boxes = api->GetComponentImages(RIL_PARA, true, NULL, NULL);

	// 文書画像から段落を抽出する
	for (int i = 0; i < para_boxes->n; i++) {
		BOX* box = boxaGetBox(para_boxes, i, L_CLONE); //boxes->n .. number of box in ptr array
		// 分割した範囲を書き出す
		Rect rect(box->x, box->y, box->w, box->h);
		Mat part_img(src, rect);
		mat_para_img = part_img;
		imwrite("../image/splitImages/para.png", part_img);
	}

	// 抜き出した段落画像をTesseractで認識させる
	Mat pw_map = mat_para_img.clone();
	Pix *para_img = pixRead("../image/splitImages/para.png");
	TessBaseAPI *api2 = new TessBaseAPI();
	api2->Init(NULL, "eng");
	api2->SetImage(para_img);
	Boxa* word_boxes = api2->GetComponentImages(RIL_WORD, true, NULL, NULL); //単語
	for (int i = 0; i < word_boxes->n; i++){
		BOX* box = boxaGetBox(word_boxes, i, L_CLONE);
		//rectangle(pw_map, Point(box->x, box->y), Point(box->x + box->w, box->y + box->h), Scalar(0, 0, 255), 1, 4);
		//imwrite("../image/splitImages/map_word.png", pw_map);
	}

	Mat valid_map = mat_para_img.clone();

	// 明らかに誤認識している単語を消去する
	Boxa* valid_boxes = boxaCreate(word_boxes->n);
	for (int i = 0; i < word_boxes->n; i++) {
		BOX* box = boxaGetBox(word_boxes, i, L_CLONE);
		if (box->h > 5 && box->w > 5){ // 閾値の調整が必要 **
			boxaAddBox(valid_boxes, box, L_CLONE);
		}
	}

	// 単語単位での認識結果を書き出す
	for (int i = 0; i < valid_boxes->n; i++) {
		BOX* box = boxaGetBox(valid_boxes, i, L_CLONE);
		api2->SetRectangle(box->x, box->y, box->w, box->h);
		char* ocrResult = api2->GetUTF8Text();
		int conf = api2->MeanTextConf();
		content << i << "," << box->x << "," << box->y << "," << box->w << "," << box->h << endl;
		//outputPartImage(box, "../image/splitImages/word_", mat_para_img, i);
		//rectangle(valid_map, Point(box->x, box->y), Point(box->x + box->w, box->y + box->h), Scalar(0, 0, 255), 1, 4);
		//imwrite("../image/splitImages/map_word_valid.png", valid_map);
	}

	findMode(valid_boxes);
}