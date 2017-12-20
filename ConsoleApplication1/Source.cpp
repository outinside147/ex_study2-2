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
ofstream fls("../image/long_images/line_spacing.txt");

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
		color = Scalar(255, 0, 255); //マゼンタ
		break;
	}
	return color;
}


// 最頻値を求める , 入力=全単語列(valid_boxes)
double findMode(Boxa* boxes){
	double class_num = ceil(1+log2((double)boxes->n)); //階級の数 スタージェスの公式: class_num = 1+log2n round=最も近い整数値に丸める
	Boxa* sort_asc = boxaSort(boxes, L_SORT_BY_HEIGHT, L_SORT_INCREASING, NULL); //昇順ソート
	Boxa* sort_desc = boxaSort(boxes, L_SORT_BY_HEIGHT, L_SORT_DECREASING, NULL); //降順ソート
	int min_h = 0;
	min_h = boxaGetBox(sort_asc, 0, L_CLONE)->h; //高さの最低値を取得する
	int max_h = 0;
	max_h = boxaGetBox(sort_desc, 0, L_CLONE)->h; //高さの最高値を取得する
	double dstrb = 0;
	dstrb = double(max_h - min_h) / class_num; //分布の間隔

	printf("boxes->n=%d, class_num=%3lf, max_h=%3d, min_h=%3d, distribution=%lf\n", boxes->n,class_num, max_h, min_h, dstrb);

	double rank = 0;
	vector<int> hist(class_num);
	for (int i = 0; i < boxes->n; i++){
		BOX* box = boxaGetBox(boxes, i, L_CLONE);
		rank = floor(box->h / dstrb); //小数点以下切り捨て
		//printf("%lf\n", rank);
		if (0 <= rank && rank < class_num){
			hist[rank]++;
		}
	}

	/*
	for (int i = 0; i < class_num; i++){
		printf("\n%lf-%lf : %3d人", double((i*dstrb) + 0.1), double((i + 1)*dstrb), hist.at(i));
	}
	printf("\n");
	*/

	int max = 0;
	int max_i = 0;
	for (size_t i = 0; i < hist.size(); i++){
		if (hist.at(i) > max){
			max = hist.at(i);
			max_i = i;
		}
	}

	double line_h = 0; //一行の高さ
	double rank_min, rank_max; //階級幅の上限と下限
	rank_min = (max_i*dstrb) + 0.1; //下限
	rank_max = (max_i + 1)*dstrb; //上限
	line_h = (rank_min + rank_max) / 2; //上限と下限の平均をとる
	printf("rank : min=%lf, max=%lf, line_h=%lf\n", rank_min, rank_max, line_h);
	return line_h*2.5; //一行の高さの2.5倍(二行の高さ)を返り値とする。
}

// 行間を見つける
void findLineSpacing(Mat pro_img,Mat l_img,int num){ //入力= 投影画像(Mat),単語画像(Mat)
	int up_edge = 0;
	int bt_edge = 0;
	Mat map = l_img.clone();

	for (int i = 0; i < pro_img.size().height; i++){
		if (pro_img.at<int>(i, 0)/255 == l_img.size().width){ //i行0列の値が単語画像の幅と同じであれば
			if(up_edge == 0) up_edge = i; //行間の上端を取得
			bt_edge = i; //行間の下端を取得
		}
	}
	//上端から文字を囲う
	rectangle(map, Point(0, 0), Point(l_img.size().width, up_edge), Scalar(0, 0, 255), 1, 1);
	//下端から文字を囲う
	rectangle(map, Point(0, bt_edge), Point(l_img.size().width, l_img.size().height), Scalar(0, 0, 255), 1, 1);
	fls << "i=" << num << endl;
	fls << "up_box=(0, 0) -- (" << l_img.size().width << ", " << up_edge << ")" << endl;
	fls << "bt_box=(0, " << bt_edge << ") -- (" << l_img.size().width << ", " << l_img.size().height << ")" << endl << endl;
	//imwrite("../image/long_images/map_ls_" + to_string(num) + ".png", map);
}

// 縦長の画像を分割する
void divideImage(Boxa* boxes,Mat img){
	ofstream pjt("../image/long_images/projection.txt");
	ofstream lng("../image/long_images/long.txt");
	ofstream gry("../image/long_images/gry.txt");

	Mat gray_img; //グレースケール画像
	Mat bn_img; //二値化画像

	cvtColor(img,gray_img,CV_RGB2GRAY); //元画像をグレースケール画像に変更する
	threshold(gray_img, bn_img, 0, 255, THRESH_BINARY | THRESH_OTSU); //大津の方法で二値化する
	//imwrite("../image/long_images/bn_image.png", bn_img);

	for (int i = 0; i < boxes->n; i++){
		BOX* box = boxaGetBox(boxes, i, L_CLONE);
		Rect rect(box->x, box->y, box->w, box->h);
		Mat long_img(bn_img, rect);
		lng << "i=" << i << ", long_img.width=" << long_img.size().width << ", long_img.height=" << long_img.size().height << ", long_img=" << endl << long_img/255 << endl;
		Mat project_img; //投影結果
		//imwrite("../image/long_images/long_" + to_string(i) + ".png", long_img);
		reduce(long_img, project_img, 1, CV_REDUCE_SUM, CV_32S); //列ごとの合計を求める,出力はint型
		pjt << "i=" << i << ", long_img.width=" << long_img.size().width << ", long_img.height=" << long_img.size().height << ", project_img=" << endl << project_img/255 << endl;
		findLineSpacing(project_img, long_img,i);
	}
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
	Mat long_map = mat_para_img.clone();

	// 明らかに誤認識している単語を消去する
	Boxa* valid_boxes = boxaCreate(word_boxes->n);
	Boxa* long_boxes = boxaCreate(word_boxes->n);

	for (int i = 0; i < word_boxes->n; i++) {
		BOX* box = boxaGetBox(word_boxes, i, L_CLONE);
		if (box->h > 5 && box->w > 5){ // 閾値の調整が必要 **
			boxaAddBox(valid_boxes, box, L_CLONE);
			if (box->h > 50){
				boxaAddBox(long_boxes, box, L_CLONE); //縦長の検出枠を格納する
			}
		}
	}

	// 単語単位での認識結果を書き出す
	for (int i = 0; i < valid_boxes->n; i++) {
		BOX* box = boxaGetBox(valid_boxes, i, L_CLONE);
		api2->SetRectangle(box->x, box->y, box->w, box->h);
		char* ocrResult = api2->GetUTF8Text();
		int conf = api2->MeanTextConf();
		content << i << "," << box->x << "," << box->y << "," << box->w << "," << box->h << endl;
		outputPartImage(box, "../image/splitImages/word_", mat_para_img, i);
		rectangle(valid_map, Point(box->x, box->y), Point(box->x + box->w, box->y + box->h), Scalar(0, 0, 255), 1, 4);
		imwrite("../image/splitImages/map_word_valid.png", valid_map);
	}

	divideImage(long_boxes, mat_para_img);

	double two_line_value = findMode(valid_boxes);
	//最頻値とその場所を表示
	printf("two_line_value=%lf\n", two_line_value);


}