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
	if (box1 == NULL || box2 == NULL) return FALSE;
	return box1->x == box2->x &&
		box1->y == box2->y &&
		box1->w == box2->w &&
		box1->h == box2->h;
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

BOX* getMinBox(BOX* min_box, BOX* box){
	if (box == NULL) return min_box;
	if (box->x < min_box->x && box->y < min_box->y){
		min_box = box;
	}
	return min_box;
}

int main()
{
	// 結果をテキストに出力
	ofstream ofs("../image/component.txt");
	ofstream mlog("../image/mlog.txt");

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
	Boxa* para_boxes = api->GetComponentImages(RIL_PARA, true, NULL, NULL);			//段落
	Boxa* line_boxes = api->GetComponentImages(RIL_TEXTLINE, true, NULL, NULL);		//行
	Boxa* word_boxes = api->GetComponentImages(RIL_WORD, true, NULL, NULL);			//単語

	for (int i = 0; i < para_boxes->n; i++) {
		BOX* box = boxaGetBox(para_boxes, i, L_CLONE); //boxes->n .. number of box in ptr array
		lparam_end = box->x; // 段落の左端
		rparam_end = box->x + box->w; //段落の右端

		api->SetRectangle(box->x, box->y, box->w, box->h);

		// 認識結果をテキストファイルとして出力する
		ofs << "Para_Box[" << i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << endl << endl;
	}

	// 行単位での認識結果を書き出す
	for (int i = 0; i < line_boxes->n; i++) {
		BOX* box = boxaGetBox(line_boxes, i, L_CLONE);
		api->SetRectangle(box->x, box->y, box->w, box->h);
	}

	// 最小値(最も左上に位置する)を格納する変数
	Box* min_box = boxCreate(src.cols, src.rows, 0, 0);

	// 1行の単語列 (文章の左上から調査して取得した順で単語を格納する配列) 
	Boxa* correct_boxes = boxaCreate(50);

	// 明らかに誤認識している単語を消去する
	Boxa* suit_Wboxes = boxaCreate(line_boxes->n);
	for (int i = 0; i < word_boxes->n; i++) {
		BOX* box = boxaGetBox(word_boxes, i, L_CLONE);
		if (box->h > 3 && box->w > 3){
			// 最も左上(最小)の座標を持つ単語領域(基準値)を抽出する
			min_box = getMinBox(min_box, box);
			boxaAddBox(suit_Wboxes, box, L_CLONE);
		}
	}
	// 最小の単語を1行目の基準値として保存する
	boxaAddBox(correct_boxes, min_box, L_CLONE);

	printf("init : min_box->x=%3d, min_box->y=%3d, min_box->w=%3d, min_box->h=%3d\n", min_box->x, min_box->y, min_box->w, min_box->h);

	// 単語単位での認識結果を書き出す
	for (int i = 0; i < suit_Wboxes->n; i++) {
		BOX* box = boxaGetBox(suit_Wboxes, i, L_CLONE);
		api->SetRectangle(box->x, box->y, box->w, box->h);
		char* ocrResult = api->GetUTF8Text();
		int conf = api->MeanTextConf();
		ofs << "Word_Box[" << i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << ", confidence=" << conf << ", text= " << ocrResult << endl;
	}

	int i = 0;
	int break_flg = 0;

	// 各行を格納する配列 Boxa型のcorrect_boxesを格納
	Boxaa* correct_lines = boxaaCreate(100);
	// 単語をすべて走査するまでループ
	while (i < 3){
		BOX* box = boxaGetBox(suit_Wboxes, i, L_CLONE);
		printf("x=%d,y=%d\n", box->x, box->y);
		// 得たボックスが空の場合の処理を追加 **
		// 基準点から右方向に単語を探索
		for (int j = min_box->x + min_box->w; j <= rparam_end; j++){
			for (int k = 0; k <= min_box->y + min_box->h; k++){
				if (j == box->x && k == box->y){
					// この行に含まれる単語として配列に格納
					boxaAddBox(correct_boxes, box, L_CLONE);
					printf("Add : i=%3d , min : x=%3d, y=%3d, w=%3d, h=%3d\n", i, box->x, box->y, box->w, box->h);
					// 比較対象がなくなった場合、その操作を行わない
					if (i < 2){
						box = boxaGetBox(suit_Wboxes, i + 1, L_CLONE);
						break_flg = 1;
					}
					break;
				}
			}
			// ループを脱する
			if (break_flg == 1) break;
		}
		if (break_flg == 0){ // 単語が見つからずにループを脱した場合
			// この場合、文章の右端まで走査し終えた。よって次の行の左上端の座標を見つける
			// これまでに見つけた単語を除いたものから、最小(最も左上)の座標にある単語を見つける
			min_box->x = src.cols;
			min_box->y = src.rows;
			min_box->w = 0;
			min_box->h = 0;
			for (int m = 0; m < suit_Wboxes->n; m++) {
				// 文字を格納する配列
				BOX* org_box = boxaGetBox(suit_Wboxes, m, L_CLONE);
				for (int p = 0; p < correct_boxes->n; p++){
					// 抽出済の文字を格納する配列
					BOX* c_box = boxaGetBox(correct_boxes, p, L_CLONE);
					// 全配列と抽出した配列を比較し、含まれていない場合は比較対象とする
					// Date_equal関数を呼び出して、その結果がfalseの場合のみ最小値の更新を行う
					if (!Date_equal(org_box, c_box)){
						// 最小値の座標を取得する
						min_box = getMinBox(min_box, org_box);
					}
				}
			}
			i++;
			printf("flg=%d, i=%3d , min_box=(%3d,%3d,%3d,%3d)\n", break_flg, i, min_box->x,min_box->y,min_box->w,min_box->h);

		}
		else if (break_flg == 1){ // 単語が見つかってループを脱した場合
			printf("i=%3d , min : x=%3d, y=%3d, w=%3d, h=%3d\n", i, box->x, box->y, box->w, box->h);
			mlog << "i=" << i << ", min : x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << endl;
			// 見つけた単語の座標を次の基準値に指定
			min_box = box;
			printf("flg=%d, i=%3d , min_box=(%3d,%3d,%3d,%3d)\n", break_flg, i, min_box->x, min_box->y, min_box->w, min_box->h);
			// フラグを初期化
			break_flg = 0;
			// 次の単語を探す
			i++;
		}
	}
	
	printf("correct_boxes->n = %3d\n", correct_boxes->n);
	for (int i = 0; i < correct_boxes->n; i++) {
		BOX* box = boxaGetBox(correct_boxes, i, L_CLONE);
		printf("i=%2d, box->x=%3d, box->y=%3d, box->w=%3d, box->h=%3d\n", i, box->x, box->y, box->w, box->h);
	}
}