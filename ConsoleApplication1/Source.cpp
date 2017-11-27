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

// 2つのBOXを比較する関数
bool  Date_equal(BOX* box1, BOX* box2){
	//if (box1 == NULL || box2 == NULL) return false;
	return box1->x == box2->x && box1->y == box2->y && box1->w == box2->w && box1->h == box2->h;
}

// 最も小さい(左上に位置する)単語を求める関数
BOX* getMinBox(BOX* min_box, BOX* box){
	if (box == NULL) return min_box;
	if (box->x <= min_box->x && box->y <= min_box->y){
		min_box = box;
	}
	return min_box;
}

BOX* getMinBox(BOX* min_box, Boxa* boxes){
	for (int i = 0; i < boxes->n; i++) {
		BOX* box = boxaGetBox(boxes, i, L_CLONE);
		if (box->x < min_box->x && box->y < min_box->y){
			min_box = box;
		}
	}
	return min_box;
}

// 次の行の先頭を探す関数
BOX* getNextRow(BOX* min_box, Boxa* boxes){
	int nearest_y = 9999;
	for (int i = 0; i < boxes->n; i++){
		BOX* box = boxaGetBox(boxes, i, L_CLONE);
		// 現在の基準値より左側に位置する
		if (box->x < min_box->x){
			// 現在の基準値より下側に位置する
			if (box->y > min_box->y){
				// その中でも現在の基準値から最もY方向に近くに位置する
				if (box->y < nearest_y){
					nearest_y = box->y;
					min_box = box;
				}
			}
		}
	}
	return min_box;
}

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
	// 結果をテキストに出力
	ofstream content("../image/component.txt");
	ofstream ex_boxes("../image/extracted_boxes.txt");
	ofstream uv_boxes("../image/unvisit_boxes.txt");
	ofstream result("../image/result.txt");

	Mat src = imread("../../../source_images/img1_3_2.jpg", 1);
	Pix *image = pixRead("../../../source_images/img1_3_2.jpg");
	Mat para_map = src.clone();
	Mat line_map = src.clone();
	Mat word_map = src.clone();

	int lparam_end = 0;
	int rparam_end = 0;

	TessBaseAPI *api = new TessBaseAPI();
	api->Init(NULL, "eng");
	api->SetImage(image);
	Boxa* para_boxes = api->GetComponentImages(RIL_PARA, true, NULL, NULL); //段落
	Boxa* line_boxes = api->GetComponentImages(RIL_TEXTLINE, true, NULL, NULL); //行
	Boxa* word_boxes = api->GetComponentImages(RIL_WORD, true, NULL, NULL); //単語
	printf("Found textline=%3d , word=%3d\n", line_boxes->n, word_boxes->n);

	for (int i = 0; i < para_boxes->n; i++) {
		BOX* box = boxaGetBox(para_boxes, i, L_CLONE); //boxes->n .. number of box in ptr array
		lparam_end = box->x; // 段落の左端
		rparam_end = box->x + box->w; //段落の右端
		api->SetRectangle(box->x, box->y, box->w, box->h);
	}

	// 行単位での認識結果を書き出す
	for (int i = 0; i < line_boxes->n; i++) {
		BOX* box = boxaGetBox(line_boxes, i, L_CLONE);
		api->SetRectangle(box->x, box->y, box->w, box->h);
	}


	Box* min_box = boxCreate(src.cols, src.rows, 0, 0); // 最小値(最も左上に位置する)を格納する変数
	Boxa* extracted_boxes = boxaCreate(50); // 1行の単語列 (文章の左上から調査して取得した順で単語を格納する配列)
	//Boxa* unvisit_boxes = boxaCreate(50); // 未走査の単語列を格納する配列
	Boxa* unvisit_boxes = nullptr;

	// 明らかに誤認識している単語を消去する
	Boxa* valid_boxes = boxaCreate(line_boxes->n);
	for (int i = 0; i < word_boxes->n; i++) {
		BOX* box = boxaGetBox(word_boxes, i, L_CLONE);
		if (box->h > 3  && box->w > 3){
			// 最も左上(最小)の座標を持つ単語領域(基準値)を抽出する
			min_box = getMinBox(min_box, box);
			boxaAddBox(valid_boxes, box, L_CLONE);
		}
	}
	// 最小の単語を1行目の基準値として保存する
	boxaAddBox(extracted_boxes,min_box,L_CLONE);
	printf("init min_box : i=0 , box=(%3d,%3d) , valid_boxes->n=%3d\n", min_box->x, min_box->y, valid_boxes->n);
	printf("lparam_end=%4d, rparam_end=%4d\n", lparam_end, rparam_end);

	// 単語単位での認識結果を書き出す
	for (int i = 0; i < valid_boxes->n; i++) {
		BOX* box = boxaGetBox(valid_boxes, i, L_CLONE);
		api->SetRectangle(box->x, box->y, box->w, box->h);
		char* ocrResult = api->GetUTF8Text();
		int conf = api->MeanTextConf();
		content << "Word_Box[" << i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << ", confidence=" << conf << ", text= " << ocrResult << endl;
	}

	int i = 0;
	int break_flg = 0;
	int nearest_y = 999;

	// 各行を格納する配列 Boxa型のextracted_boxesを格納
	// 編集ここから **
	Boxaa* extracted_lines = boxaaCreate(100);
	// 単語をすべて走査するまでループ
	for (int i = 1; i < valid_boxes->n; i++){
		BOX* box = boxaGetBox(valid_boxes, i, L_CLONE);
		// 得たボックスが空の場合の処理を追加 **
		// 基準点から右方向に単語を探索
		for (int j = min_box->x + min_box->w; j <= rparam_end; j++){
			for (int k = 0; k <= min_box->y + min_box->h; k++){
				if (j == box->x && k == box->y){
					printf("word extracted : i=%3d , box=(%3d,%3d)\n", i, j, k);
					// この行に含まれる単語として配列に格納
					boxaAddBox(extracted_boxes, box, L_CLONE);
					break_flg = 1;
					break;
				}
			} // ループを脱する
			if (break_flg == 1) break;
		}

		// 基準値か右方向に単語が存在しなかった場合
		if (break_flg == 0){
			// これまでに見つけた単語を除いた中から、最小(最も左上に位置する)の単語を見つける。それを次の基準値とする
			unvisit_boxes = boxaCreate(50);
			set_difference(valid_boxes, extracted_boxes, unvisit_boxes);
			printf("unvisit_boxes->n=%3d\n",unvisit_boxes->n);
			min_box = getNextRow(min_box, unvisit_boxes);
			printf("not extracted : i=%3d\n", i);
			printf("next row point : min_box=(%3d,%3d)\n", min_box->x, min_box->y);
			boxaAddBox(extracted_boxes, min_box, L_CLONE);
			result << "break_flg=" << break_flg << ", i=" << i << ", box= (" << box->x << "," << box->y << ")" << ", next point= (" << min_box->x << "," << min_box->y << ")" << endl;
		}

		// 単語が見つかってループを脱した場合
		if (break_flg == 1){
			// 見つけた単語の座標を次の基準値に指定
			min_box = box;
			printf("next word point : min_box=(%3d,%3d)\n", min_box->x, min_box->y);
			result << "break_flg=" << break_flg << ", i=" << i << ", box= (" << box->x << "," << box->y << ")" << ", next point= (" << min_box->x << "," << min_box->y << ")" << endl;
			// フラグを初期化
			break_flg = 0;
		}
	}

	printf("extracted_boxes->n=%3d\n", extracted_boxes->n);
	ex_boxes << "extracted_boxes->n=" << extracted_boxes->n << endl;
	for (int i = 0; i < extracted_boxes->n; i++) {
		BOX* box = boxaGetBox(extracted_boxes, i, L_CLONE);
		ex_boxes << "i=" << i << ", box= (" << box->x << "," << box->y << "," << box->w << "," << box->h << ")" << endl;
	}

	printf("unvisit_boxes->n=%3d\n", unvisit_boxes->n);
	uv_boxes << "unvisit_boxes->n=" << unvisit_boxes->n << endl;
	for (int i = 0; i < unvisit_boxes->n; i++) {
		BOX* box = boxaGetBox(unvisit_boxes, i, L_CLONE);
		uv_boxes << "i=" << i << ", box= (" << box->x << "," << box->y << "," << box->w << "," << box->h << ")" << endl;
	}
}