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
Mat src = imread("../../../source_images/img1_3_2.jpg", 1);
Pix *image = pixRead("../../../source_images/img1_3_2.jpg");
//Mat src = imread("../../../source_images/vrs_orientation/front_tr.jpg", 1); //入力画像
//Pix *image = pixRead("../../../source_images/vrs_orientation/front_tr.jpg"); //入力画像(tesseract,leptonicaで使用する型)
ofstream fls("../image/long_images/line_spacing.txt");


// BOX型の中心点を格納する構造体
typedef struct { double x, y; } Box_array;

// 行間の上端と下端を格納する構造体
typedef struct { int up, bt; } Bet_lines;

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
	Box_array vec = { 0, 0 };
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

// 全ての行の先頭単語を見つける
Boxa* setStartPosition(Boxa* boxes){
	ofstream sortx("../image/sorted_x.txt");
	ofstream sorty("../image/sorted_y.txt");
	ofstream lboxes("../image/leading_boxes.txt");

	Boxa* sort_xboxes = boxaCreate(500);
	Boxa* sort_yboxes = boxaCreate(100);
	Boxa* leading_boxes = boxaCreate(100);

	Mat xsort_map = src.clone();
	Mat ysort_map = src.clone();
	Mat leading_map = src.clone();

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
		rectangle(ysort_map, Point(box->x, box->y), Point(box->x + box->w, box->y + box->h), Scalar(255, 255, 0), 1, 4);
		//imwrite("../image/splitImages/map_word_ysort.png", ysort_map);
		//sorty << "sorted_y[" << i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << endl;
	}

	int rcnt = 1;
	BOX* min_box = boxaGetBox(sort_yboxes, 0, L_CLONE);
	//ソートされた単語について、同じ行に複数単語が候補として抽出されている場合、最も左にある単語をその行の先頭単語とする
	while (rcnt < sort_yboxes->n){
		BOX* cur_box = boxaGetBox(sort_yboxes, rcnt, L_CLONE);
		int dif_y = cur_box->y - min_box->y;
		//2つの単語間の長さが30以上ある場合、先頭単語の配列に格納する
		if (dif_y >= 30){
			boxaAddBox(leading_boxes, min_box, L_CLONE);
			min_box = cur_box;
		}
		//2つの単語間の長さが30未満の場合、最もXが小さいものを取得する
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
		//lboxes << "leading_boxes[" << i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << endl;
		rectangle(leading_map, Point(box->x, box->y), Point(box->x + box->w, box->y + box->h), Scalar(0, 0, 255), 1, 4);
		//imwrite("../image/splitImages/map_word_leading.png", leading_map);
	}
	return leading_boxes;
}


// 行の先頭単語から右方向にある単語を探す
Boxa* findFollowWord(BOX* l_box, Boxa* v_boxes){
	Box* leading_box = l_box; //先頭単語
	Boxa* all_boxes = v_boxes; //全単語列
	Boxa* line_boxes = boxaCreate(500); //探索済みの単語列
	BOX* st_point = boxCreate(0, 0, 0, 0);; //一つ目の単語
	BOX* ed_point = boxCreate(0, 0, 0, 0);; //二つ目の単語
	BOX* next_word = boxCreate(0, 0, 0, 0);; //次の単語
	Box_array vec = { 0, 0 };	//2つの単語を結ぶベクトル
	Box_array base_vec = { 1, 0 }; //box2=(1,0) , X軸方向のベクトル

	double min_x = 0;
	double vec_size1 = 0; //ベクトルの大きさ1
	double vec_size2 = 0; //ベクトルの大きさ2
	double vec_cos = 0; //2つのベクトルのなす角度
	double base_angle = cos(10.0 * PI / 180.0); // 次の単語を同じ行と判定する基準角度
	printf("base_angle=%f\n", base_angle);

	//探索を始める初期値を設定
	st_point = leading_box;
	printf("init : st_point=(%3d,%3d)\n", st_point->x, st_point->y);
	//先頭単語を探索済み配列に格納
	boxaAddBox(line_boxes, st_point, L_CLONE);

	while (1){
		min_x = MAX_X;
		for (int j = 0; j < all_boxes->n; j++){
			ed_point = boxaGetBox(all_boxes, j, L_CLONE);
			//2つの単語の中心座標を取得し、そのベクトルを求める
			vec = getVector(st_point, ed_point);
			Box_array st_center = getCenterPoint(st_point); //開始位置の中心
			Box_array ed_center = getCenterPoint(ed_point); //終端位置の中心

			//X軸方向のベクトルと上記で求めたベクトルのなす角度を求める
			//各ベクトルの大きさを取得する
			vec_size1 = getVectorLength(vec);
			vec_size2 = getVectorLength(base_vec);

			//cosθを求める , cosθ=内積/ (√ベクトルの大きさ1*√ベクトルの大きさ2)
			vec_cos = getProduct(vec, base_vec) / (vec_size1 * vec_size2);

			//cosθが10度から-10度であれば右方向にあると判断する
			if (vec_cos >= base_angle){
				//角度が10度から-10度の範囲にある、かつ最も近い(X座標が)単語を次の単語とする
				if (ed_center.x - st_center.x < min_x){
					min_x = ed_center.x - st_center.x;
					next_word = ed_point;
					printf("ed_center.x - st_center.x=%lf, next_word=(%d,%d,%d,%d)\n", ed_center.x - st_center.x, next_word->x, next_word->y, next_word->w, next_word->h);
				}
			}
		}

		//右方向に単語が見つからなくなればループを脱する
		if (min_x == MAX_X){
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
	return line_boxes;
}

// 一行分の高さを求める , 入力=全単語列
//全単語列の高さでヒストグラムを作成する。ヒストグラムから最頻値を求め、最頻値のビンの区間中央値を一行分の高さとする。
double findMode(Boxa* boxes){
	double class_num = ceil(1 + log2((double)boxes->n)); //階級の数 スタージェスの公式: class_num = 1+log2n round=最も近い整数値に丸める
	Boxa* sort_asc = boxaSort(boxes, L_SORT_BY_HEIGHT, L_SORT_INCREASING, NULL); //昇順ソート
	Boxa* sort_desc = boxaSort(boxes, L_SORT_BY_HEIGHT, L_SORT_DECREASING, NULL); //降順ソート
	int min_h = boxaGetBox(sort_asc, 0, L_CLONE)->h; //高さの最低値を取得する
	int max_h = boxaGetBox(sort_desc, 0, L_CLONE)->h; //高さの最高値を取得する
	double dstrb = double(max_h - min_h) / class_num; //分布の間隔

	printf("boxes->n=%d, class_num=%3lf, max_h=%3d, min_h=%3d, distribution=%lf\n", boxes->n, class_num, max_h, min_h, dstrb);

	double rank = 0; //ビン
	vector<int> hist(class_num); //ヒストグラム
	for (int i = 0; i < boxes->n; i++){
		BOX* box = boxaGetBox(boxes, i, L_CLONE);
		rank = floor(box->h / dstrb); //ビンに割り当てる。小数点以下切り捨て
		if (0 <= rank && rank < class_num){
			hist[rank]++;
		}
	}

	int max = 0;
	int max_i = 0;
	for (size_t i = 0; i < hist.size(); i++){
		if (hist.at(i) > max){
			max = hist.at(i); //最頻値の要素数を求める
			max_i = i; //最頻値の場所を取得
		}
	}

	double line_h = 0; //一行の高さ
	double rank_min, rank_max; //階級幅の上限と下限
	rank_min = (max_i*dstrb) + 0.1; //下限
	rank_max = (max_i + 1)*dstrb; //上限
	line_h = (rank_min + rank_max) / 2; //上限と下限の平均をとる
	printf("rank : min=%lf, max=%lf, line_h=%lf\n", rank_min, rank_max, line_h);
	printf("return value = %lf\n", line_h * 2);
	return line_h * 2; //一行の高さの2倍(二行の高さ)を返り値とする。
}

// 行間を見つける
Bet_lines findLineSpacing(Mat pro_img, Mat word_img, int num){ //入力= 投影画像(Mat),単語画像(Mat)
	int up_edge = 0;
	int bt_edge = 0;
	int max = 0; //空白行が連続している最大
	int space_cnt = 0; //空白行をカウントする変数
	int st_space = 0;
	Mat map = word_img.clone();

	printf("num=%d\n", num);
	for (int i = 0; i < pro_img.size().height; i++){
		if (pro_img.at<int>(i, 0) == 0){ //i行の値が0(空白)であればその場所を記録
			space_cnt++;
			if (space_cnt > max){
				max = space_cnt;
				if (st_space == 0) st_space = i; //行間の上端を取得
			}
		}
		else {
			space_cnt = 0;
		}
	}
	up_edge = st_space;
	bt_edge = st_space + max;
	printf("up_edge=%d, bt_edge=%d\n", up_edge, bt_edge);

	//行間の上端、下端が見つからなかった場合、単語を縦方向に2分割する
	if (up_edge == 0 && bt_edge == 0){
		up_edge = word_img.size().height / 2;
		bt_edge = word_img.size().height / 2;
	}
	else{
		bt_edge--;
	}

	//結果をファイルへ出力
	//fls << "i=" << num << endl;
	//fls << "up_box=(0, 0) -- (" << word_img.size().width << ", " << up_edge << "), up_box.h=" << up_edge << endl;
	//fls << "bt_box=(0, " << bt_edge << ") -- (" << word_img.size().width << ", " << word_img.size().height << "), bt_box.h=" << word_img.size().height - bt_edge << endl << endl;
	//imwrite("../image/long_images/map_ls_" + to_string(num) + ".png", map);

	Bet_lines edge = { up_edge, bt_edge };
	return  edge;
}

// 縦長の画像を分割する
Boxa* divideImage(Boxa* boxes, Mat img){
	ofstream pjt("../image/long_images/projection.txt");
	ofstream lng("../image/long_images/long.txt");

	Mat gray_img; //グレースケール画像
	Mat bn_img; //二値化画像

	Boxa* edge_box = boxaCreate(100); //分割した抽出枠を格納する配列

	cvtColor(img, gray_img, CV_RGB2GRAY); //元画像をグレースケール画像に変更する
	threshold(gray_img, bn_img, 0, 255, THRESH_BINARY_INV | THRESH_OTSU); //大津の方法で求めた閾値以上であれば0、それ以下であれば255
	//imwrite("../image/long_images/bn_image.png", bn_img);

	for (int i = 0; i < boxes->n; i++){
		BOX* box = boxaGetBox(boxes, i, L_CLONE);
		Rect rect(box->x, box->y, box->w, box->h);
		Mat long_img(bn_img, rect);
		//lng << "i=" << i << ", long_img.width=" << long_img.size().width << ", long_img.height=" << long_img.size().height << ", long_img=" << endl << long_img / 255 << endl;
		Mat project_img; //投影結果
		reduce(long_img, project_img, 1, CV_REDUCE_SUM, CV_32S); //列ごとの合計を求める,出力はint型
		//pjt << "i=" << i << ", long_img.width=" << long_img.size().width << ", long_img.height=" << long_img.size().height << ", project_img=" << endl << project_img / 255 << endl;
		Bet_lines edge = findLineSpacing(project_img, long_img, i);
		BOX* up_box = boxCreate(box->x, box->y, box->w, edge.up);
		BOX* bt_box = boxCreate(box->x, box->y + edge.bt, box->w, box->h - edge.bt);
		boxaAddBox(edge_box, up_box, L_CLONE);
		boxaAddBox(edge_box, bt_box, L_CLONE);
	}
	return edge_box;
}

/*
// 同じ単語を含む行配列を見つける
Boxaa* findDeplication(Boxaa* boxas){
for (int i = 0; i < boxas->n; i++){
Boxa* boxes = boxaaGetBoxa(boxas, i, L_CLONE);
for (int j = 0; j < boxes->n; j++){
BOX* box = boxaGetBox(boxes, j, L_CLONE);

}
}

}
*/

int main()
{
	ofstream content("../image/component.txt");
	ofstream tgt_content("../image/tgt_component.txt");

	TessBaseAPI *api = new TessBaseAPI();
	api->Init(NULL, "eng");
	api->SetImage(image);

	Mat word_map = src.clone();
	Boxa* word_boxes = api->GetComponentImages(RIL_WORD, true, NULL, NULL);
	for (int i = 0; i < word_boxes->n; i++){
		BOX* box = boxaGetBox(word_boxes, i, L_CLONE);
		rectangle(word_map, Point(box->x, box->y), Point(box->x + box->w, box->y + box->h), Scalar(255, 0, 0), 1, 4);
		//imwrite("../image/splitImages/map_word.png", word_map);
	}

	Mat valid_map = src.clone();
	Mat sentence_map = src.clone();

	Boxa* valid_boxes = boxaCreate(word_boxes->n); //小さな抽出枠を取り除いた単語列
	Boxa* long_boxes = boxaCreate(word_boxes->n); //縦長の単語列
	Boxa* div_boxes = boxaCreate(word_boxes->n); //分割単語列
	Boxa* tgt_boxes = boxaCreate(word_boxes->n * 2); //valid_boxesと縦長の単語列を分割した単語列を合わせたもの
	Boxa* leading_boxes = boxaCreate(200); //先頭単語列
	Boxa* line_boxes = boxaCreate(200); //一行の単語列
	Boxaa* sentence_boxas = boxaaCreate(200); //全行の単語列

	// 明らかに誤認識している単語を消去する
	for (int i = 0; i < word_boxes->n; i++) {
		BOX* box = boxaGetBox(word_boxes, i, L_CLONE);
		if (box->h > 5 && box->w > 5){ //極端に小さい検出枠は除外
			boxaAddBox(valid_boxes, box, L_CLONE);
		}
	}

	// 単語単位での認識結果を書き出す
	for (int i = 0; i < valid_boxes->n; i++) {
		BOX* box = boxaGetBox(valid_boxes, i, L_CLONE);
		api->SetRectangle(box->x, box->y, box->w, box->h);
		char* ocrResult = api->GetUTF8Text();
		int conf = api->MeanTextConf();
		//content << "Word_Box[" << i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << ", confidence=" << conf << ", text= " << ocrResult << endl;
		//outputPartImage(box, "../image/splitImages/word_", src, i);
		rectangle(valid_map, Point(box->x, box->y), Point(box->x + box->w, box->y + box->h), Scalar(0, 0, 255), 1, 4);
		//imwrite("../image/splitImages/map_word_valid.png", valid_map);
	}

	// 二行に渡る抽出枠を縦方向に分割する
	double two_row_length = findMode(valid_boxes); //二行の長さを設定
	//最頻値とその場所を表示
	printf("two_row_length=%lf\n", two_row_length);
	for (int i = 0; i < valid_boxes->n; i++) { //上記の値を使って、二行の長さの検出枠を抽出する
		BOX* box = boxaGetBox(valid_boxes, i, L_CLONE);
		if (box->h > two_row_length){
			boxaAddBox(long_boxes, box, L_CLONE);
		}
	}

	//縦長の画像を分割する
	div_boxes = divideImage(long_boxes, src); //縦長の画像を分割する

	//分割後の画像を探索配列に格納する
	for (int i = 0; i < div_boxes->n; i++){
		BOX* box = boxaGetBox(div_boxes, i, L_CLONE);
		if (box->h > 10){
			boxaAddBox(tgt_boxes, box, L_CLONE);
		}
	}
	//分割前の単語(縦長の画像)を探索対象から外す
	for (int i = 0; i < valid_boxes->n; i++){
		BOX* box = boxaGetBox(valid_boxes, i, L_CLONE);
		if (box->h < two_row_length){
			boxaAddBox(tgt_boxes, box, L_CLONE);
		}
	}

	Mat all_map = src.clone();
	//全単語を出力する
	for (int i = 0; i < tgt_boxes->n; i++){
		BOX* box = boxaGetBox(tgt_boxes, i, L_CLONE);
		rectangle(all_map, Point(box->x, box->y), Point(box->x + box->w, box->y + box->h), Scalar(255, 0, 255), 1, 1);
		imwrite("../image/splitImages/all_map_word.png", all_map);
		//outputPartImage(box, "../image/splitImages/target_boxes/tgt_", src, i);
		//tgt_content << "Word_Box[" << i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << endl;
	}

	// 全ての行の先頭単語を見つける , 入力=全単語列
	leading_boxes = setStartPosition(tgt_boxes);

	// 先頭単語の数だけ右方向の探索処理を行う
	for (int i = 0; i < leading_boxes->n; i++){
		BOX* box = boxaGetBox(leading_boxes, i, L_CLONE);
		//先頭単語から右方向に向かって次の単語を探す , 入力=先頭単語,全単語列
		line_boxes = findFollowWord(box, tgt_boxes);
		boxaaAddBoxa(sentence_boxas, line_boxes, L_CLONE);
		//valid_boxesからline_boxesの要素を取り除いて返す。これを次の探索対象とする , 重複フラグ
		//valid_boxes = setDiff(valid_boxes, line_boxes);
	}

	printf("sentence_boxas->n=%3d\n", sentence_boxas->n);
	// 全行の単語列を書き出す
	ofstream s_boxas("../image/sentence_boxas.txt");
	for (int i = 0; i < sentence_boxas->n; i++){
		//s_boxas << i + 1 << endl;
		Boxa* boxes = boxaaGetBoxa(sentence_boxas, i, L_CLONE);
		for (int j = 0; j < boxes->n; j++){
			BOX* box = boxaGetBox(boxes, j, L_CLONE);
			Rect rect(box->x, box->y, box->w, box->h);
			Mat part_img(src, rect);
			//imwrite("../image/splitImages/s_boxas_" + to_string(i) + "_" + to_string(j) + ".png", part_img);
			int ci = i % 4;
			rectangle(sentence_map, Point(box->x, box->y), Point(box->x + box->w, box->y + box->h), setColor(ci), 1, 4);
			imwrite("../image/splitImages/map_sentence.png", sentence_map);
			//imwrite("../image/splitImages/mapsentence/map_sentence_" + to_string(i) + ".png", sentence_map);
			//s_boxas << "sentence_boxas[" << i << "][" << j << "],x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << endl;
			//s_boxas << i << "," << j << "," << box->x << "," << box->y << "," << box->w << "," << box->h << endl;
		}
	}
}