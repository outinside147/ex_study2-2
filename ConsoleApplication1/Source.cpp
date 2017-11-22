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

// 2��BOX���r����֐�
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
	// ���ʂ��e�L�X�g�ɏo��
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
	Boxa* para_boxes = api->GetComponentImages(RIL_PARA, true, NULL, NULL);			//�i��
	Boxa* line_boxes = api->GetComponentImages(RIL_TEXTLINE, true, NULL, NULL);		//�s
	Boxa* word_boxes = api->GetComponentImages(RIL_WORD, true, NULL, NULL);			//�P��

	for (int i = 0; i < para_boxes->n; i++) {
		BOX* box = boxaGetBox(para_boxes, i, L_CLONE); //boxes->n .. number of box in ptr array
		lparam_end = box->x; // �i���̍��[
		rparam_end = box->x + box->w; //�i���̉E�[

		api->SetRectangle(box->x, box->y, box->w, box->h);

		// �F�����ʂ��e�L�X�g�t�@�C���Ƃ��ďo�͂���
		ofs << "Para_Box[" << i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << endl << endl;
	}

	// �s�P�ʂł̔F�����ʂ������o��
	for (int i = 0; i < line_boxes->n; i++) {
		BOX* box = boxaGetBox(line_boxes, i, L_CLONE);
		api->SetRectangle(box->x, box->y, box->w, box->h);
	}

	// �ŏ��l(�ł�����Ɉʒu����)���i�[����ϐ�
	Box* min_box = boxCreate(src.cols, src.rows, 0, 0);

	// 1�s�̒P��� (���͂̍��ォ�璲�����Ď擾�������ŒP����i�[����z��) 
	Boxa* correct_boxes = boxaCreate(50);

	// ���炩�Ɍ�F�����Ă���P�����������
	Boxa* suit_Wboxes = boxaCreate(line_boxes->n);
	for (int i = 0; i < word_boxes->n; i++) {
		BOX* box = boxaGetBox(word_boxes, i, L_CLONE);
		if (box->h > 3 && box->w > 3){
			// �ł�����(�ŏ�)�̍��W�����P��̈�(��l)�𒊏o����
			min_box = getMinBox(min_box, box);
			boxaAddBox(suit_Wboxes, box, L_CLONE);
		}
	}
	// �ŏ��̒P���1�s�ڂ̊�l�Ƃ��ĕۑ�����
	boxaAddBox(correct_boxes, min_box, L_CLONE);

	printf("init : min_box->x=%3d, min_box->y=%3d, min_box->w=%3d, min_box->h=%3d\n", min_box->x, min_box->y, min_box->w, min_box->h);

	// �P��P�ʂł̔F�����ʂ������o��
	for (int i = 0; i < suit_Wboxes->n; i++) {
		BOX* box = boxaGetBox(suit_Wboxes, i, L_CLONE);
		api->SetRectangle(box->x, box->y, box->w, box->h);
		char* ocrResult = api->GetUTF8Text();
		int conf = api->MeanTextConf();
		ofs << "Word_Box[" << i << "]: x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << ", confidence=" << conf << ", text= " << ocrResult << endl;
	}

	int i = 0;
	int break_flg = 0;

	// �e�s���i�[����z�� Boxa�^��correct_boxes���i�[
	Boxaa* correct_lines = boxaaCreate(100);
	// �P������ׂđ�������܂Ń��[�v
	while (i < 3){
		BOX* box = boxaGetBox(suit_Wboxes, i, L_CLONE);
		printf("x=%d,y=%d\n", box->x, box->y);
		// �����{�b�N�X����̏ꍇ�̏�����ǉ� **
		// ��_����E�����ɒP���T��
		for (int j = min_box->x + min_box->w; j <= rparam_end; j++){
			for (int k = 0; k <= min_box->y + min_box->h; k++){
				if (j == box->x && k == box->y){
					// ���̍s�Ɋ܂܂��P��Ƃ��Ĕz��Ɋi�[
					boxaAddBox(correct_boxes, box, L_CLONE);
					printf("Add : i=%3d , min : x=%3d, y=%3d, w=%3d, h=%3d\n", i, box->x, box->y, box->w, box->h);
					// ��r�Ώۂ��Ȃ��Ȃ����ꍇ�A���̑�����s��Ȃ�
					if (i < 2){
						box = boxaGetBox(suit_Wboxes, i + 1, L_CLONE);
						break_flg = 1;
					}
					break;
				}
			}
			// ���[�v��E����
			if (break_flg == 1) break;
		}
		if (break_flg == 0){ // �P�ꂪ�����炸�Ƀ��[�v��E�����ꍇ
			// ���̏ꍇ�A���͂̉E�[�܂ő������I�����B����Ď��̍s�̍���[�̍��W��������
			// ����܂łɌ������P������������̂���A�ŏ�(�ł�����)�̍��W�ɂ���P���������
			min_box->x = src.cols;
			min_box->y = src.rows;
			min_box->w = 0;
			min_box->h = 0;
			for (int m = 0; m < suit_Wboxes->n; m++) {
				// �������i�[����z��
				BOX* org_box = boxaGetBox(suit_Wboxes, m, L_CLONE);
				for (int p = 0; p < correct_boxes->n; p++){
					// ���o�ς̕������i�[����z��
					BOX* c_box = boxaGetBox(correct_boxes, p, L_CLONE);
					// �S�z��ƒ��o�����z����r���A�܂܂�Ă��Ȃ��ꍇ�͔�r�ΏۂƂ���
					// Date_equal�֐����Ăяo���āA���̌��ʂ�false�̏ꍇ�̂ݍŏ��l�̍X�V���s��
					if (!Date_equal(org_box, c_box)){
						// �ŏ��l�̍��W���擾����
						min_box = getMinBox(min_box, org_box);
					}
				}
			}
			i++;
			printf("flg=%d, i=%3d , min_box=(%3d,%3d,%3d,%3d)\n", break_flg, i, min_box->x,min_box->y,min_box->w,min_box->h);

		}
		else if (break_flg == 1){ // �P�ꂪ�������ă��[�v��E�����ꍇ
			printf("i=%3d , min : x=%3d, y=%3d, w=%3d, h=%3d\n", i, box->x, box->y, box->w, box->h);
			mlog << "i=" << i << ", min : x=" << box->x << ", y=" << box->y << ", w=" << box->w << ", h=" << box->h << endl;
			// �������P��̍��W�����̊�l�Ɏw��
			min_box = box;
			printf("flg=%d, i=%3d , min_box=(%3d,%3d,%3d,%3d)\n", break_flg, i, min_box->x, min_box->y, min_box->w, min_box->h);
			// �t���O��������
			break_flg = 0;
			// ���̒P���T��
			i++;
		}
	}
	
	printf("correct_boxes->n = %3d\n", correct_boxes->n);
	for (int i = 0; i < correct_boxes->n; i++) {
		BOX* box = boxaGetBox(correct_boxes, i, L_CLONE);
		printf("i=%2d, box->x=%3d, box->y=%3d, box->w=%3d, box->h=%3d\n", i, box->x, box->y, box->w, box->h);
	}
}