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
#define MAX_X 9999

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

BOX* create_box(int x, int y, int w, int h){
	BOX* box = boxCreate(x, y, w, h);
	return box;
}

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


// 全ての行の先頭単語を見つける
Boxa* setStartPosition(Boxa* boxes){
	ofstream sortx("../image/sorted_x.txt");
	ofstream sorty("../image/sorted_y.txt");
	ofstream lboxes("../image/leading_boxes.txt");

	Boxa* sort_xboxes = boxaCreate(500);
	Boxa* sort_yboxes = boxaCreate(100);
	Boxa* leading_boxes = boxaCreate(100);

	Mat word_map2 = mat_para_img.clone();
	Mat word_map3 = mat_para_img.clone();

	//xの昇順でソート
	sort_xboxes = boxaSort(boxes, L_SORT_BY_X, L_SORT_INCREASING, NULL);
	for (int i = 0; i < sort_xboxes->n; i++){
		BOX* box = boxaGetBox(sort_xboxes, i, L_CLONE);
		//sortx << "sorted_x[" << i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << endl;
	}

	//ソートした配列の先頭から100個の単語を行の先頭候補として別の配列に格納する
	for (int i = 0; i < 100; i++){
		BOX* box = boxaGetBox(sort_xboxes, i, L_CLONE);
		boxaAddBox(sort_yboxes, box, L_CLONE);
	}

	//yの昇順でソート
	sort_yboxes = boxaSort(sort_yboxes, L_SORT_BY_Y, L_SORT_INCREASING, NULL);
	for (int i = 0; i < sort_yboxes->n; i++){
		BOX* box = boxaGetBox(sort_yboxes, i, L_CLONE);
		rectangle(word_map2, Point(box->x, box->y), Point(box->x + box->w, box->y + box->h), Scalar(255, 255, 0), 1, 4);
		imwrite("../image/splitImages/map_word_ysort.png", word_map2);
		sorty << "sorted_y[" << i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << endl;
	}

	int rcnt = 1;
	BOX* min_box = boxaGetBox(sort_yboxes, 0, L_CLONE);
	//ソートされた単語について、同じ行に複数単語が候補として抽出されている場合、最も左にある単語をその行の先頭単語とする
	while (rcnt < sort_yboxes->n){
		BOX* cur_box = boxaGetBox(sort_yboxes, rcnt, L_CLONE);
		int dif_y = cur_box->y - min_box->y;
		//printf("cur_box=(%3d,%3d), dif_y=%3d, min_box=(%3d,%3d)\n", cur_box->x, cur_box->y, dif_y, min_box->x,min_box->y);
		//単語の高さが30以上ある場合、先頭単語の配列に格納する
		if (dif_y >= 30){
			boxaAddBox(leading_boxes, min_box, L_CLONE);
			min_box = cur_box;
		}
		//単語の高さが30未満の場合、最もXが小さいものを取得する
		else if (dif_y < 30){
			if (cur_box->x < min_box->x){
				min_box = cur_box;
			}
		}
		rcnt++;
	}
	boxaAddBox(leading_boxes, min_box, L_CLONE);

	// 抽出した先頭単語を出力する
	for (int i = 0; i < leading_boxes->n; i++){
		BOX* box = boxaGetBox(leading_boxes, i, L_CLONE);
		lboxes << "leading_boxes[" << i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << endl;
		rectangle(word_map3, Point(box->x, box->y), Point(box->x + box->w, box->y + box->h), Scalar(0, 0, 255), 1, 4);
		imwrite("../image/splitImages/map_word_leading.png", word_map3);
	}
	return leading_boxes;
}


// 行の先頭単語から右方向にある単語を探す
Boxa* findFollowWord(BOX* l_box,Boxa* v_boxes){
	Box* leading_box = l_box; //先頭単語
	Boxa* valid_boxes = v_boxes; //全単語列
	Boxa* line_boxes = boxaCreate(500); //探索済みの単語列
	BOX* st_point = boxCreate(0, 0, 0, 0);; //一つ目の単語
	BOX* ed_point = boxCreate(0, 0, 0, 0);; //二つ目の単語
	BOX* next_word = boxCreate(0, 0, 0, 0);; //次の単語
	Box_array vec = { 0, 0 };	//2つの単語を結ぶベクトル
	Box_array base_vec = { 1, 0 }; //box2=(1,0) , X軸方向のベクトル

	int min_x;
	int lbox_cnt = 0;
	double vec_size1; //ベクトルの大きさ1
	double vec_size2; //ベクトルの大きさ2
	double vec_cos; //2つのベクトルのなす角度
	double base_angle = cos(20.0 * PI / 180.0); // 次の単語を同じ行と判定する基準角度
	printf("base_angle=%f\n", base_angle);

	//探索を始める初期値を設定
	st_point = leading_box;
	printf("init : st_point=(%3d,%3d)\n", st_point->x, st_point->y);
	//先頭単語を探索済み配列に格納
	boxaAddBox(line_boxes, st_point, L_CLONE);

	while (1){
		min_x = MAX_X;
		for (int j = 0; j < valid_boxes->n; j++){
			ed_point = boxaGetBox(valid_boxes, j, L_CLONE);
			//printf("st_point=(%3d,%3d),ed_point=(%3d,%3d)\n", st_point->x, st_point->y, ed_point->x, ed_point->y);
			//2つの単語の中心座標を取得し、そのベクトルを求める
			vec = getVector(st_point, ed_point);

			//X軸方向のベクトルと上記で求めたベクトルのなす角度を求める
			//各ベクトルの大きさを取得する
			vec_size1 = getVectorLength(vec);
			vec_size2 = getVectorLength(base_vec);

			//cosθを求める , cosθ=内積/ (√ベクトルの大きさ1*√ベクトルの大きさ2)
			vec_cos = getProduct(vec, base_vec) / (vec_size1 * vec_size2);

			//cosθが1/2(=30度から-30度)であれば右方向にあると判断する
			if (vec_cos >= base_angle){
				//角度が30度から-30度の範囲にある、かつ最も近い(X座標が)単語を次の単語とする
				if (ed_point->x - st_point->x < min_x){
					min_x = ed_point->x - st_point->x;
					next_word = ed_point;
					Box_array stc = getCenterPoint(st_point);
					Box_array edc = getCenterPoint(ed_point);
					//printf("st_point=(%3d,%3d,%3d,%3d),ed_point=(%3d,%3d,%3d,%3d), min_x=%3d ,vec_cos=%lf\n", st_point->x, st_point->y, st_point->w, st_point->h, ed_point->x, ed_point->y, ed_point->w, ed_point->h, min_x, vec_cos);
					//printf("st_center=(%lf,%lf) , ed_center=(%lf,%lf)\n", stc.x, stc.y, edc.x, edc.y);
				}
			}
		}

		//右方向に単語が見つからなくなれば次の行へ移動する
		if (min_x == MAX_X){
			printf("go to next row\n");
			break;
		}
		else {
			//次の単語を始点に設定する
			st_point = next_word;
			printf("Add : st_point=(%3d,%3d)\n", st_point->x, st_point->y);
			//st_pointを探索済みの配列に代入する
			boxaAddBox(line_boxes, st_point, L_CLONE);
		}
	}
	printf("exit a roop\n");
	return line_boxes;
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
		rectangle(pw_map, Point(box->x, box->y), Point(box->x + box->w, box->y + box->h), Scalar(0, 0, 255), 1, 4);
		imwrite("../image/splitImages/map_word.png", pw_map);
	}

	Mat word_map1 = mat_para_img.clone();
	Mat word_map4 = mat_para_img.clone();
	Boxa* leading_boxes = boxaCreate(100); //先頭単語列
	Boxa* line_boxes = boxaCreate(100); //一行の単語列
	Boxaa* sentence_boxas = boxaaCreate(100); //全行の単語列


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
		content << "Word_Box[" << i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << ", confidence=" << conf << ", text= " << ocrResult << endl;
		//outputPartImage(box, "../image/splitImages/word_", mat_para_img, i);
		rectangle(word_map1, Point(box->x, box->y), Point(box->x + box->w, box->y + box->h), Scalar(0, 0, 255), 1, 4);
		imwrite("../image/splitImages/map_word_valid.png", word_map1);
	}

	// 全ての行の先頭単語を見つける , 入力=全単語列
	leading_boxes = setStartPosition(valid_boxes);

	for (int i = 0; i < leading_boxes->n; i++){
		BOX* box = boxaGetBox(leading_boxes, i, L_CLONE);
		Boxa* searched_boxes = boxaCreate(500);
		//先頭単語から右方向に向かって次の単語を探す , 入力=先頭単語,全単語列
		line_boxes = findFollowWord(box, valid_boxes);
		boxaaAddBoxa(sentence_boxas, line_boxes, L_CLONE);
		//valid_boxesからline_boxesの要素を取り除いて返す。これを次の探索対象とする
		valid_boxes = setDiff(valid_boxes, line_boxes);
	}

	printf("sentence_boxas->n=%3d\n", sentence_boxas->n);
	// 全行の単語列を書き出す
	ofstream s_boxas("../image/sentence_boxas.txt");
	for (int i = 0; i < sentence_boxas->n; i++){
		Boxa* boxes = boxaaGetBoxa(sentence_boxas,i,L_CLONE);
		for (int j = 0; j < boxes->n;j++){
			BOX* box = boxaGetBox(boxes, j, L_CLONE);
			Rect rect(box->x, box->y, box->w, box->h);
			Mat part_img(mat_para_img, rect);
			imwrite("../image/splitImages/s_boxas_" + to_string(i) + "_" + to_string(j) + ".png", part_img);
			int ci = i % 4;
			rectangle(word_map4, Point(box->x, box->y), Point(box->x + box->w, box->y + box->h), setColor(ci), 1, 4);
			imwrite("../image/splitImages/map_sentence.png", word_map4);
			s_boxas << "sentence_boxas[" << i << "][" << j << "],x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << endl;
		}
	}
}