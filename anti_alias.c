#include <string.h>
#include "types.h"
#include "anti_alias.h"
#include "layer.h"
#include "application.h"

#ifdef _OPENMP
# include <omp.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************
* OldAntiAlias�֐�                                        *
* �A���`�G�C���A�V���O���������s                       *
* ����                                                 *
* in_buff		: ���̓f�[�^                           *
* out_buff		: �o�̓f�[�^                           *
* width			: ���o�̓f�[�^�̕�                     *
* height		: ���o�̓f�[�^�̍���                   *
* stride		: ���o�̓f�[�^��1�s���̃o�C�g��        *
* application	: �A�v���P�[�V�����S�̂��Ǘ�����\���� *
*******************************************************/
void OldAntiAlias(
	uint8* in_buff,
	uint8* out_buff,
	int width,
	int height,
	int stride,
	void* application
)
{
// �A���`�G�C���A�V���O���s��1�`�����l������臒l
#define ANTI_ALIAS_THRESHOLD (51*51)
#define MINIMUM_PARALLEL_SIZE 50
#define CHANNEL (4)

	const APPLICATION *app = (APPLICATION*)application;
	// �A���`�G�C���A�V���O���s��臒l
	const int threshold = ANTI_ALIAS_THRESHOLD * CHANNEL;
	// �s�N�Z���f�[�^�z��̃C���f�b�N�X
	int index, now_index;
	// ���݂̃s�N�Z���Ǝ��͂̃s�N�Z���̐F��
	int color_diff[4];
	// �F���̍��v
	int sum_diff;
	// ���͂̃s�N�Z���̍��v�l
	int sum_color[4];
	int i;	// for���p�̃J�E���^

	// 1�s�ڂ͂��̂܂܃R�s�[
	(void)memcpy(out_buff, in_buff, stride);

#ifdef _OPENMP
	if(height <= MINIMUM_PARALLEL_SIZE)
	{
		omp_set_dynamic(FALSE);
		omp_set_num_threads(1);
	}
# pragma omp parallel for firstprivate(width, in_buff, out_buff)
#endif
	// 2�s�ڂ����ԉ���O�̍s�܂ŏ���
	for(i=1; i<height-1; i++)
	{
		int j, k;	// for���p�̃J�E���^

		// ��ԍ��͂��̂܂܃R�s�[
		index = i * stride;
		for(j=0; j<CHANNEL; j++)
		{
			out_buff[i+j] = in_buff[i+j];
		}

		for(j=1; j<width-1; j++)
		{	// �F�����v���N���A
			color_diff[3] = color_diff[2] = color_diff[1] = color_diff[0] = 0;
			// ���͂̃s�N�Z���̍��v�l���N���A
			sum_color[3] = sum_color[2] = sum_color[1] = sum_color[0] = 0;

			// ����8�s�N�Z���Ƃ̐F���ƒl�̍��v���v�Z
			now_index = i*stride + j * CHANNEL;
			index = (i-1)*stride + (j-1) * CHANNEL;
			for(k=0; k<CHANNEL; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[now_index+k];
				sum_color[k] += in_buff[index+k];
			}
			index = (i-1)*stride + j * CHANNEL;
			for(k=0; k<CHANNEL; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}
			index = (i-1)*stride + (j+1) * CHANNEL;
			for(k=0; k<CHANNEL; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}
			index = i*stride + (j-1) * CHANNEL;
			for(k=0; k<CHANNEL; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}
			index = i*stride + (j+1) * CHANNEL;
			for(k=0; k<CHANNEL; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}
			index = (i+1)*stride + (j-1) * CHANNEL;
			for(k=0; k<CHANNEL; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}
			index = (i+1)*stride + j * CHANNEL;
			for(k=0; k<CHANNEL; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}
			index = (i+1)*stride + (j+1) * CHANNEL;
			for(k=0; k<CHANNEL; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}

			sum_diff = color_diff[0];
			for(k=1; k<CHANNEL; k++)
			{
				sum_diff += color_diff[k];
			}

			// �F���̍��v��臒l�ȏ�Ȃ�΃A���`�G�C���A�V���O���s
			if(sum_diff > threshold)
			{	// �ݒ肷��V���ȃf�[�^
				uint8 new_value;

				for(k=0; k<CHANNEL; k++)
				{
					new_value = (uint8)(sum_color[k] / 9);
					if(new_value > in_buff[now_index+k])
					{
						out_buff[now_index+k] = new_value;
					}
					else
					{
						out_buff[now_index+k] = in_buff[now_index+k];
					}
				}
			}
			else
			{
				for(k=0; k<CHANNEL; k++)
				{
					out_buff[now_index+k] = in_buff[now_index+k];
				}
			}
		}
	}	// 2�s�ڂ����ԉ���O�̍s�܂ŏ���
		// for(i=1; i<height-1; i++)
#ifdef _OPENMP
	omp_set_dynamic(TRUE);
	omp_set_num_threads(app->max_threads);
#endif

	// ��ԉ��̍s�͂��̂܂܃R�s�[
	(void)memcpy(&out_buff[(height-1)*stride], &in_buff[(height-1)*stride], stride);
#undef ANTI_ALIAS_THRESHOLD
#undef CHANNEL
}

/*******************************************************
* OldAntiAliasEditSelection�֐�                        *
* �N�C�b�N�}�X�N���̃A���`�G�C���A�V���O���������s     *
* ����                                                 *
* in_buff		: ���̓f�[�^                           *
* out_buff		: �o�̓f�[�^                           *
* width			: ���o�̓f�[�^�̕�                     *
* height		: ���o�̓f�[�^�̍���                   *
* stride		: ���o�̓f�[�^��1�s���̃o�C�g��        *
* application	: �A�v���P�[�V�����S�̂��Ǘ�����\���� *
*******************************************************/
void OldAntiAliasEditSelection(
	uint8* in_buff,
	uint8* out_buff,
	int width,
	int height,
	int stride,
	void* application
)
{
// �A���`�G�C���A�V���O���s��1�`�����l������臒l
#define ANTI_ALIAS_THRESHOLD (51*51)
#define MINIMUM_PARALLEL_SIZE 50
#define CHANNEL (1)

	const APPLICATION *app = (APPLICATION*)application;
	// �A���`�G�C���A�V���O���s��臒l
	const int threshold = ANTI_ALIAS_THRESHOLD * CHANNEL;
	// �s�N�Z���f�[�^�z��̃C���f�b�N�X
	int index, now_index;
	// ���݂̃s�N�Z���Ǝ��͂̃s�N�Z���̐F��
	int color_diff[4];
	// �F���̍��v
	int sum_diff;
	// ���͂̃s�N�Z���̍��v�l
	int sum_color[4];
	int i;	// for���p�̃J�E���^

	// 1�s�ڂ͂��̂܂܃R�s�[
	(void)memcpy(out_buff, in_buff, stride);

#ifdef _OPENMP
	if(height <= MINIMUM_PARALLEL_SIZE)
	{
		omp_set_dynamic(FALSE);
		omp_set_num_threads(1);
	}
# pragma omp parallel for firstprivate(width, in_buff, out_buff)
#endif
	// 2�s�ڂ����ԉ���O�̍s�܂ŏ���
	for(i=1; i<height-1; i++)
	{
		int j, k;	// for���p�̃J�E���^

		// ��ԍ��͂��̂܂܃R�s�[
		index = i * stride;
		for(j=0; j<CHANNEL; j++)
		{
			out_buff[i+j] = in_buff[i+j];
		}

		for(j=1; j<width-1; j++)
		{	// �F�����v���N���A
			color_diff[3] = color_diff[2] = color_diff[1] = color_diff[0] = 0;
			// ���͂̃s�N�Z���̍��v�l���N���A
			sum_color[3] = sum_color[2] = sum_color[1] = sum_color[0] = 0;

			// ����8�s�N�Z���Ƃ̐F���ƒl�̍��v���v�Z
			now_index = i*stride + j * CHANNEL;
			index = (i-1)*stride + (j-1) * CHANNEL;
			for(k=0; k<CHANNEL; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[now_index+k];
				sum_color[k] += in_buff[index+k];
			}
			index = (i-1)*stride + j * CHANNEL;
			for(k=0; k<CHANNEL; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}
			index = (i-1)*stride + (j+1) * CHANNEL;
			for(k=0; k<CHANNEL; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}
			index = i*stride + (j-1) * CHANNEL;
			for(k=0; k<CHANNEL; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}
			index = i*stride + (j+1) * CHANNEL;
			for(k=0; k<CHANNEL; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}
			index = (i+1)*stride + (j-1) * CHANNEL;
			for(k=0; k<CHANNEL; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}
			index = (i+1)*stride + j * CHANNEL;
			for(k=0; k<CHANNEL; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}
			index = (i+1)*stride + (j+1) * CHANNEL;
			for(k=0; k<CHANNEL; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}

			sum_diff = color_diff[0];
			for(k=1; k<CHANNEL; k++)
			{
				sum_diff += color_diff[k];
			}

			// �F���̍��v��臒l�ȏ�Ȃ�΃A���`�G�C���A�V���O���s
			if(sum_diff > threshold)
			{	// �ݒ肷��V���ȃf�[�^
				uint8 new_value;

				for(k=0; k<CHANNEL; k++)
				{
					new_value = (uint8)(sum_color[k] / 9);
					if(new_value > in_buff[now_index+k])
					{
						out_buff[now_index+k] = new_value;
					}
					else
					{
						out_buff[now_index+k] = in_buff[now_index+k];
					}
				}
			}
			else
			{
				for(k=0; k<CHANNEL; k++)
				{
					out_buff[now_index+k] = in_buff[now_index+k];
				}
			}
		}
	}	// 2�s�ڂ����ԉ���O�̍s�܂ŏ���
		// for(i=1; i<height-1; i++)
#ifdef _OPENMP
	omp_set_dynamic(TRUE);
	omp_set_num_threads(app->max_threads);
#endif

	// ��ԉ��̍s�͂��̂܂܃R�s�[
	(void)memcpy(&out_buff[(height-1)*stride], &in_buff[(height-1)*stride], stride);
#undef ANTI_ALIAS_THRESHOLD
#undef CHANNEL
}

/*************************************************************
* OldAntiAliasWithSelectionOrAlphaLock�֐�                   *
* �A���`�G�C���A�V���O���������s(�I��͈͂܂��͓����x�ی�L) *
* ����                                                       *
* selection_layer	: �I��͈�                               *
* target_layer		: �����x�ی�ɑΏۃ��C���[               *
* in_buff			: ���̓f�[�^                             *
* out_buff			: �o�̓f�[�^                             *
* start_x			: �A���`�G�C���A�X�������鍶���X���W    *
* start_y			: �A���`�G�C���A�X�������鍶���Y���W    *
* width				: ���o�̓f�[�^�̕�                       *
* height			: ���o�̓f�[�^�̍���                     *
* stride			: ���o�̓f�[�^��1�s���̃o�C�g��          *
* application		: �A�v���P�[�V�����S�̂��Ǘ�����\����   *
*************************************************************/
void OldAntiAliasWithSelectionOrAlphaLock(
	LAYER* selection_layer,
	LAYER* target_layer,
	uint8* in_buff,
	uint8* out_buff,
	int start_x,
	int start_y,
	int width,
	int height,
	int stride,
	void* application
)
{
// �A���`�G�C���A�V���O���s��1�`�����l������臒l
#define ANTI_ALIAS_THRESHOLD (51*51)
#define MINIMUM_PARALLEL_SIZE 50
#define CHANNEL (4)

	const APPLICATION *app = (APPLICATION*)application;
	// �A���`�G�C���A�V���O���s��臒l
	const int threshold = ANTI_ALIAS_THRESHOLD * CHANNEL;
	// �s�N�Z���f�[�^�z��̃C���f�b�N�X
	int index, now_index;
	// ���݂̃s�N�Z���Ǝ��͂̃s�N�Z���̐F��
	int color_diff[4];
	// �F���̍��v
	int sum_diff;
	// ���͂̃s�N�Z���̍��v�l
	int sum_color[4];
	int i;	// for���p�̃J�E���^

	// 1�s�ڂ͂��̂܂܃R�s�[
	(void)memcpy(out_buff, in_buff, stride);

#ifdef _OPENMP
	if(height <= MINIMUM_PARALLEL_SIZE)
	{
		omp_set_dynamic(FALSE);
		omp_set_num_threads(1);
	}
# pragma omp parallel for firstprivate(width, in_buff, out_buff)
#endif
	// 2�s�ڂ����ԉ���O�̍s�܂ŏ���
	for(i=1; i<height-1; i++)
	{
		int j, k;	// for���p�̃J�E���^

		// ��ԍ��͂��̂܂܃R�s�[
		index = i * stride;
		for(j=0; j<CHANNEL; j++)
		{
			out_buff[i+j] = in_buff[i+j];
		}

		for(j=1; j<width-1; j++)
		{	// �F�����v���N���A
			color_diff[3] = color_diff[2] = color_diff[1] = color_diff[0] = 0;
			// ���͂̃s�N�Z���̍��v�l���N���A
			sum_color[3] = sum_color[2] = sum_color[1] = sum_color[0] = 0;

			// ����8�s�N�Z���Ƃ̐F���ƒl�̍��v���v�Z
			now_index = i*stride + j * CHANNEL;
			index = (i-1)*stride + (j-1) * CHANNEL;
			for(k=0; k<CHANNEL; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[now_index+k];
				sum_color[k] += in_buff[index+k];
			}
			index = (i-1)*stride + j * CHANNEL;
			for(k=0; k<CHANNEL; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}
			index = (i-1)*stride + (j+1) * CHANNEL;
			for(k=0; k<CHANNEL; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}
			index = i*stride + (j-1) * CHANNEL;
			for(k=0; k<CHANNEL; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}
			index = i*stride + (j+1) * CHANNEL;
			for(k=0; k<CHANNEL; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}
			index = (i+1)*stride + (j-1) * CHANNEL;
			for(k=0; k<CHANNEL; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}
			index = (i+1)*stride + j * CHANNEL;
			for(k=0; k<CHANNEL; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}
			index = (i+1)*stride + (j+1) * CHANNEL;
			for(k=0; k<CHANNEL; k++)
			{
				color_diff[k] += ((int)in_buff[now_index+k]-(int)in_buff[index+k])
					* ((int)in_buff[now_index+k]-(int)in_buff[index+k]);
				sum_color[k] += in_buff[index+k];
			}

			sum_diff = color_diff[0];
			for(k=1; k<CHANNEL; k++)
			{
				sum_diff += color_diff[k];
			}

			// �F���̍��v��臒l�ȏ�Ȃ�΃A���`�G�C���A�V���O���s
			if(sum_diff > threshold)
			{	// �ݒ肷��V���ȃf�[�^
				uint8 new_value;

				for(k=0; k<CHANNEL; k++)
				{
					new_value = (uint8)(sum_color[k] / 9);
					if(new_value > in_buff[now_index+k])
					{
						out_buff[now_index+k] = new_value;
					}
					else
					{
						out_buff[now_index+k] = in_buff[now_index+k];
					}
				}
			}
			else
			{
				for(k=0; k<CHANNEL; k++)
				{
					out_buff[now_index+k] = in_buff[now_index+k];
				}
			}
		}
	}	// 2�s�ڂ����ԉ���O�̍s�܂ŏ���
		// for(i=1; i<height-1; i++)

	// ��ԉ��̍s�͂��̂܂܃R�s�[
	(void)memcpy(&out_buff[(height-1)*stride], &in_buff[(height-1)*stride], stride);

	// �I��͈͂ւ̑Ή�
	if(selection_layer != NULL)
	{
#ifdef _OPENMP
# pragma omp parallel for firstprivate(width, out_buff, start_x, start_y, selection_layer)
#endif
		for(i=0; i<height; i++)
		{
			uint8 *selection = &selection_layer->pixels[(i + start_y) * selection_layer->stride + start_x];
			uint8 *out = &out_buff[stride];
			int j;
			for(j=0; j<width; j++, selection++, out+=4)
			{
				if(out[3] > *selection)
				{
					out[3] = *selection;
					if(out[0] > out[3])
					{
						out[0] = out[3];
					}
					if(out[1] > out[3])
					{
						out[1] = out[3];
					}
					if(out[2] > out[3])
					{
						out[2] = out[3];
					}
				}
			}
		}
	}

	// �����x�ی�ւ̑Ή�
	if(target_layer != NULL)
	{
#ifdef _OPENMP
# pragma omp parallel for firstprivate(width, out_buff, start_x, start_y, target_layer)
#endif
		for(i=0; i<height; i++)
		{
			uint8 *target = &target_layer->pixels[(i + start_y) * target_layer->stride + start_x * CHANNEL];
			uint8 *out = &out_buff[stride];
			int alpha;
			int j;
			for(j=0; j<width; j++, target+=CHANNEL, out+=CHANNEL)
			{
				if(target[3] < 255)
				{
					alpha = target[3] - out[3];
					if(alpha >= 0)
					{
						target[3] = target[3] - alpha;
						if(target[0] > target[3])
						{
							target[0] = target[3];
						}
						if(target[1] > target[3])
						{
							target[1] = target[3];
						}
						if(target[2] > target[3])
						{
							target[2] = target[3];
						}

						if(out[3] > alpha)
						{
							out[3] = alpha;
						}
						if(out[0] > out[3])
						{
							out[0] = out[3];
						}
						if(out[1] > out[3])
						{
							out[1] = out[3];
						}
						if(out[2] > out[3])
						{
							out[2] = out[3];
						}
					}
					else
					{
						alpha = target[3];
						target[0] = 0;
						target[1] = 0;
						target[2] = 0;
						target[3] = 0;

						if(out[3] > alpha)
						{
							out[3] = alpha;
						}
						if(out[0] > out[3])
						{
							out[0] = out[3];
						}
						if(out[1] > out[3])
						{
							out[1] = out[3];
						}
						if(out[2] > out[3])
						{
							out[2] = out[3];
						}
					}
				}
			}
		}
	}


#ifdef _OPENMP
	omp_set_dynamic(TRUE);
	omp_set_num_threads(app->max_threads);
#endif

#undef ANTI_ALIAS_THRESHOLD
#undef CHANNEL
}

static INLINE void AntiAliasPixel(uint8* pixels, uint8* out, int stride)
{
	int sum_color[4];
	uint8 *src;

	src = pixels - stride - 4;
	sum_color[0] = src[0];
	sum_color[1] = src[1];
	sum_color[2] = src[2];
	sum_color[3] = src[3];
	src += 4;
	sum_color[0] += src[0];
	sum_color[1] += src[1];
	sum_color[2] += src[2];
	sum_color[3] += src[3];
	src += 4;
	sum_color[0] += src[0];
	sum_color[1] += src[1];
	sum_color[2] += src[2];
	sum_color[3] += src[3];
	src = pixels - 4;
	sum_color[0] += src[0];
	sum_color[1] += src[1];
	sum_color[2] += src[2];
	sum_color[3] += src[3];
	src += 4;
	sum_color[0] += src[0];
	sum_color[1] += src[1];
	sum_color[2] += src[2];
	sum_color[3] += src[3];
	src += 4;
	sum_color[0] += src[0];
	sum_color[1] += src[1];
	sum_color[2] += src[2];
	sum_color[3] += src[3];
	src = pixels + stride + 4;
	sum_color[0] += src[0];
	sum_color[1] += src[1];
	sum_color[2] += src[2];
	sum_color[3] += src[3];
	src += 4;
	sum_color[0] += src[0];
	sum_color[1] += src[1];
	sum_color[2] += src[2];
	sum_color[3] += src[3];
	src += 4;
	sum_color[0] += src[0];
	sum_color[1] += src[1];
	sum_color[2] += src[2];
	sum_color[3] += src[3];

	out[0] = sum_color[0] / 9;
	out[1] = sum_color[1] / 9;
	out[2] = sum_color[2] / 9;
	out[3] = sum_color[3] / 9;
}

/*********************************************************************
* AntiAliasLayer�֐�                                                 *
* ���C���[�ɑ΂��Ĕ͈͂��w�肵�ăA���`�G�C���A�X��������             *
* layer			: �A���`�G�C���A�X�������郌�C���[                   *
* out			: ���ʂ����郌�C���[                               *
* rect			: �A���`�G�C���A�X��������͈�                       *
* applicatoion	: �A�v���P�[�V�����S�̂��Ǘ�����\���̊Ǘ�����\���� *
*********************************************************************/
void AntiAliasLayer(LAYER *layer, LAYER* out, ANTI_ALIAS_RECTANGLE *rect, void* application)
{
// �A���`�G�C���A�V���O���s��1�`�����l������臒l
#define ANTI_ALIAS_THRESHOLD ((51*51)/16)
#define MINIMUM_PARALLEL_SIZE 50
	// �A���`�G�C���A�X�J�n�E�I���̍��W
	int x, y, end_x, end_y;
	// ����1�s���̃o�C�g��
	int stride = layer->stride;
	// ��������s�N�Z��
	uint8 *pixels = layer->pixels;
	uint8 *out_pixels = out->pixels;
	int diff_red, diff_green, diff_blue, diff_alpha;
	int i;	// for���p�̃J�E���^

	// ���W�Ɣ͈͂�ݒ�
	end_x = rect->width;
	end_y = rect->height;
	if(rect->x < 0)
	{
		end_x += rect->x;
		x = 0;
	}
	else if(rect->x > layer->width)
	{
		return;
	}
	else
	{
		x = rect->x;
	}
	end_x = x + end_x;
	if(end_x > layer->width)
	{
		end_x = layer->width;
	}
	if(end_x - x < 3)
	{
		return;
	}

	if(rect->y < 0)
	{
		end_y += rect->y;
		y = 0;
	}
	else if(rect->y > layer->height)
	{
		return;
	}
	else
	{
		y = rect->y;
	}
	end_y = y + end_y;
	if(end_y > layer->height)
	{
		end_y = layer->height;
	}

	(void)memcpy(&out_pixels[y*layer->stride+x*4], &pixels[y*layer->stride+x*4],
		(end_x - x) * 4);

	// 2�s�ڂ����ԉ���O�̍s�܂ŏ���
#ifdef _OPENMP
	if(rect->height <= MINIMUM_PARALLEL_SIZE)
	{
		omp_set_dynamic(FALSE);
		omp_set_num_threads(1);
	}
#pragma omp parallel for firstprivate(pixels, out_pixels, stride, x, y, end_x)
#endif
	for(i=y+1; i<end_y-1; i++)
	{
		// �s�N�Z���f�[�^�z��̃C���f�b�N�X
		int now_index;
		uint8 alpha;	// �ݒ肷�郿�l
		int j;	// for���p�̃J�E���^

		// ��ԍ��͂��̂܂܃R�s�[
		now_index = i * stride + x * 4;
		out_pixels[now_index+0] = pixels[now_index+0];
		out_pixels[now_index+1] = pixels[now_index+1];
		out_pixels[now_index+2] = pixels[now_index+2];
		out_pixels[now_index+3] = pixels[now_index+3];

		now_index = i * stride + (x+1)*4;

		for(j=x+1; j<end_x-1; j++, now_index+=4)
		{

			// �㉺���E�̃s�N�Z���̐F�����v�Z
			diff_red = ((int)pixels[now_index-4+0]-(int)pixels[now_index-stride+0])
				* ((int)pixels[now_index-4+0]-(int)pixels[now_index-stride+0]);
			diff_green = ((int)pixels[now_index-4+1]-(int)pixels[now_index-stride+1])
				* ((int)pixels[now_index-4+1]-(int)pixels[now_index-stride+1]);
			diff_blue = ((int)pixels[now_index-4+2]-(int)pixels[now_index-stride+2])
				* ((int)pixels[now_index-4+2]-(int)pixels[now_index-stride+2]);
			diff_alpha = ((int)pixels[now_index-4+3]-(int)pixels[now_index-stride+3])
				* ((int)pixels[now_index-4+3]-(int)pixels[now_index-stride+3]);
			if(diff_red + diff_green + diff_blue + diff_alpha > ANTI_ALIAS_THRESHOLD)
			{
				alpha = (uint8)(((int)pixels[now_index+3] + (int)pixels[now_index-stride+3] + (int)pixels[now_index-4+3]) / 3);
				if(alpha > pixels[now_index+3] && alpha > out_pixels[now_index+3])
				{
					out_pixels[now_index+0] = (uint8)(((int)pixels[now_index+0] + (int)pixels[now_index-stride+0] + (int)pixels[now_index-4+0]) / 3);
					out_pixels[now_index+1] = (uint8)(((int)pixels[now_index+1] + (int)pixels[now_index-stride+1] + (int)pixels[now_index-4+1]) / 3);
					out_pixels[now_index+2] = (uint8)(((int)pixels[now_index+2] + (int)pixels[now_index-stride+2] + (int)pixels[now_index-4+2]) / 3);
					out_pixels[now_index+3] = alpha;
					//AntiAliasPixel(&pixels[now_index], &out_pixels[now_index], stride);
					continue;
				}
			}

			diff_red = ((int)pixels[now_index+4+0]-(int)pixels[now_index-stride+0])
				* ((int)pixels[now_index+4+0]-(int)pixels[now_index-stride+0]);
			diff_green = ((int)pixels[now_index+4+1]-(int)pixels[now_index-stride+1])
				* ((int)pixels[now_index+4+1]-(int)pixels[now_index-stride+1]);
			diff_blue = ((int)pixels[now_index+4+2]-(int)pixels[now_index-stride+2])
				* ((int)pixels[now_index+4+2]-(int)pixels[now_index-stride+2]);
			diff_alpha = ((int)pixels[now_index+4+3]-(int)pixels[now_index-stride+3])
				* ((int)pixels[now_index+4+3]-(int)pixels[now_index-stride+3]);
			if(diff_red + diff_green + diff_blue + diff_alpha > ANTI_ALIAS_THRESHOLD)
			{
				alpha = (uint8)(((int)pixels[now_index+3] + (int)pixels[now_index-stride+3] + (int)pixels[now_index+4+3]) / 3);
				if(alpha > pixels[now_index+3] && alpha > out_pixels[now_index+3])
				{
					out_pixels[now_index+0] = (uint8)(((int)pixels[now_index+0] + (int)pixels[now_index-stride+0] + (int)pixels[now_index+4+0]) / 3);
					out_pixels[now_index+1] = (uint8)(((int)pixels[now_index+1] + (int)pixels[now_index-stride+1] + (int)pixels[now_index+4+1]) / 3);
					out_pixels[now_index+2] = (uint8)(((int)pixels[now_index+2] + (int)pixels[now_index-stride+2] + (int)pixels[now_index+4+2]) / 3);
					out_pixels[now_index+3] = alpha;
					//AntiAliasPixel(&pixels[now_index], &out_pixels[now_index], stride);
					continue;
				}
			}

			diff_red = ((int)pixels[now_index-4+0]-(int)pixels[now_index+stride+0])
				* ((int)pixels[now_index-4+0]-(int)pixels[now_index+stride+0]);
			diff_green = ((int)pixels[now_index-4+1]-(int)pixels[now_index+stride+1])
				* ((int)pixels[now_index-4+1]-(int)pixels[now_index+stride+1]);
			diff_blue = ((int)pixels[now_index-4+2]-(int)pixels[now_index+stride+2])
				* ((int)pixels[now_index-4+2]-(int)pixels[now_index+stride+2]);
			diff_alpha = ((int)pixels[now_index-4+3]-(int)pixels[now_index+stride+3])
				* ((int)pixels[now_index-4+3]-(int)pixels[now_index+stride+3]);
			if(diff_red + diff_green + diff_blue + diff_alpha > ANTI_ALIAS_THRESHOLD)
			{
				alpha = (uint8)(((int)pixels[now_index+3] + (int)pixels[now_index+stride+3] + (int)pixels[now_index-4+3]) / 3);
				if(alpha > pixels[now_index+3] && alpha > out_pixels[now_index+3])
				{
					out_pixels[now_index+0] = (uint8)(((int)pixels[now_index+0] + (int)pixels[now_index+stride+0] + (int)pixels[now_index-4+0]) / 3);
					out_pixels[now_index+1] = (uint8)(((int)pixels[now_index+1] + (int)pixels[now_index+stride+1] + (int)pixels[now_index-4+1]) / 3);
					out_pixels[now_index+2] = (uint8)(((int)pixels[now_index+2] + (int)pixels[now_index+stride+2] + (int)pixels[now_index-4+2]) / 3);
					out_pixels[now_index+3] = alpha;
					//AntiAliasPixel(&pixels[now_index], &out_pixels[now_index], stride);
					continue;
				}
			}

			diff_red = ((int)pixels[now_index+4+0]-(int)pixels[now_index+stride+0])
				* ((int)pixels[now_index+4+0]-(int)pixels[now_index+stride+0]);
			diff_green = ((int)pixels[now_index+4+1]-(int)pixels[now_index+stride+1])
				* ((int)pixels[now_index+4+1]-(int)pixels[now_index+stride+1]);
			diff_blue = ((int)pixels[now_index+4+2]-(int)pixels[now_index+stride+2])
				* ((int)pixels[now_index+4+2]-(int)pixels[now_index+stride+2]);
			diff_alpha = ((int)pixels[now_index+4+3]-(int)pixels[now_index+stride+3])
				* ((int)pixels[now_index+4+3]-(int)pixels[now_index+stride+3]);
			if(diff_red + diff_green + diff_blue + diff_alpha > ANTI_ALIAS_THRESHOLD)
			{
				alpha = (uint8)(((int)pixels[now_index+3] + (int)pixels[now_index+stride+3] + (int)pixels[now_index+4+3]) / 3);
				if(alpha > pixels[now_index+3] && alpha > out_pixels[now_index+3])
				{
					out_pixels[now_index+0] = (uint8)(((int)pixels[now_index+0] + (int)pixels[now_index+stride+0] + (int)pixels[now_index+4+0]) / 3);
					out_pixels[now_index+1] = (uint8)(((int)pixels[now_index+1] + (int)pixels[now_index+stride+1] + (int)pixels[now_index+4+1]) / 3);
					out_pixels[now_index+2] = (uint8)(((int)pixels[now_index+2] + (int)pixels[now_index+stride+2] + (int)pixels[now_index+4+2]) / 3);
					out_pixels[now_index+3] = alpha;
					//AntiAliasPixel(&pixels[now_index], &out_pixels[now_index], stride);
					continue;
				}
			}

			if(pixels[now_index+3] > out_pixels[now_index+3])
			{
				out_pixels[now_index+0] = pixels[now_index+0];
				out_pixels[now_index+1] = pixels[now_index+1];
				out_pixels[now_index+2] = pixels[now_index+2];
				out_pixels[now_index+3] = pixels[now_index+3];
			}
		}
	}	// 2�s�ڂ����ԉ���O�̍s�܂ŏ���
		// for(i=1; i<height-1; i++)
#ifdef _OPENMP
	omp_set_dynamic(TRUE);
	omp_set_num_threads(((APPLICATION*)(application))->max_threads);
#endif

	(void)memcpy(&out_pixels[(end_y-1)*layer->stride+x*4], &pixels[(end_y-1)*layer->stride+x*4],
		(end_x - x) * 4);

#undef ANTI_ALIAS_THRESHOLD
#undef MINIMUM_PARALLEL_SIZE
}

/*********************************************************************
* OldAntiAliasLayer�֐�                                              *
* ���C���[�ɑ΂��Ĕ͈͂��w�肵�ăA���`�G�C���A�X��������             *
* layer			: �A���`�G�C���A�X�������郌�C���[                   *
* rect			: �A���`�G�C���A�X��������͈�                       *
* applicatoion	: �A�v���P�[�V�����S�̂��Ǘ�����\���̊Ǘ�����\���� *
*********************************************************************/
void OldAntiAliasLayer(LAYER *layer, LAYER* temp, ANTI_ALIAS_RECTANGLE *rect, void* application)
{
// �A���`�G�C���A�V���O���s��1�`�����l������臒l
#define ANTI_ALIAS_THRESHOLD (51*51)
#define MINIMUM_PARALLEL_SIZE 50
	// �A���`�G�C���A�X�J�n�E�I���̍��W
	int x, y, end_x, end_y;
	// ����1�s���̃o�C�g��
	int stride;
	// �������郌�C���[�̈�s���̃o�C�g��
	int layer_stride = layer->stride;
	// ��������s�N�Z��
	uint8 *pixels = layer->pixels;
	uint8 *temp_pixels = temp->pixels;
	int i;	// for���p�̃J�E���^

	// ���W�Ɣ͈͂�ݒ�
	end_x = rect->width;
	end_y = rect->height;
	if(rect->x < 0)
	{
		end_x += rect->x;
		x = 0;
	}
	else if(rect->x > layer->width)
	{
		return;
	}
	else
	{
		x = rect->x;
	}
	end_x = x + end_x;
	if(end_x > layer->width)
	{
		end_x = layer->width;
	}
	if(end_x - x < 3)
	{
		return;
	}

	if(rect->y < 0)
	{
		end_y += rect->y;
		y = 0;
	}
	else if(rect->y > layer->height)
	{
		return;
	}
	else
	{
		y = rect->y;
	}
	end_y = y + end_y;
	if(end_y > layer->height)
	{
		end_y = layer->height;
	}

	// 2�s�ڂ����ԉ���O�̍s�܂ŏ���
#ifdef _OPENMP
	if(rect->height <= MINIMUM_PARALLEL_SIZE)
	{
		omp_set_dynamic(FALSE);
		omp_set_num_threads(1);
	}
#pragma omp parallel for firstprivate(pixels, temp_pixels, layer_stride, x, y, end_x)
#endif
	for(i=y+1; i<end_y-1; i++)
	{
		// �s�N�Z���f�[�^�z��̃C���f�b�N�X
		int index, now_index;
		// ���݂̃s�N�Z���Ǝ��͂̃s�N�Z���̐F��
		int color_diff[4];
		// �F���̍��v
		int sum_diff;
		// ���͂̃s�N�Z���̍��v�l
		int sum_color[4];
		int j;	// for���p�̃J�E���^

		// ��ԍ��͂��̂܂܃R�s�[
		now_index = i * layer->stride + (x+1)*4;

		for(j=x+1; j<end_x-1; j++, now_index+=4)
		{

			// ����8�s�N�Z���Ƃ̐F���ƒl�̍��v���v�Z
			index = (i-1)*layer_stride + (j-1)*4;
			color_diff[0] = ((int)pixels[now_index+0]-(int)pixels[index+0])
				* ((int)pixels[now_index+0]-(int)pixels[index+0]);
			sum_color[0] = pixels[now_index+0];
			sum_color[0] += pixels[index+0];
			color_diff[1] = ((int)pixels[now_index+1]-(int)pixels[index+1])
				* ((int)pixels[now_index+1]-(int)pixels[index+0]);
			sum_color[1] = pixels[now_index+1];
			sum_color[1] += pixels[index+1];
			color_diff[2] = ((int)pixels[now_index+2]-(int)pixels[index+2])
				* ((int)pixels[now_index+2]-(int)pixels[index+0]);
			sum_color[2] = pixels[now_index+2];
			sum_color[2] += pixels[index+2];
			color_diff[3] = ((int)pixels[now_index+3]-(int)pixels[index+3])
				* ((int)pixels[now_index+3]-(int)pixels[index+3]);
			sum_color[3] = pixels[now_index+3];
			sum_color[3] += pixels[index+3];

			index += 4;
			color_diff[0] += ((int)pixels[now_index+0]-(int)pixels[index+0])
				* ((int)pixels[now_index+0]-(int)pixels[index+0]);
			sum_color[0] += pixels[index+0];
			color_diff[1] += ((int)pixels[now_index+1]-(int)pixels[index+1])
				* ((int)pixels[now_index+1]-(int)pixels[index+0]);
			sum_color[1] += pixels[index+1];
			color_diff[2] += ((int)pixels[now_index+2]-(int)pixels[index+2])
				* ((int)pixels[now_index+2]-(int)pixels[index+0]);
			sum_color[2] += pixels[index+2];
			color_diff[3] += ((int)pixels[now_index+3]-(int)pixels[index+3])
				* ((int)pixels[now_index+3]-(int)pixels[index+3]);
			sum_color[3] += pixels[index+3];

			index += 4;
			color_diff[0] += ((int)pixels[now_index+0]-(int)pixels[index+0])
				* ((int)pixels[now_index+0]-(int)pixels[index+0]);
			sum_color[0] += pixels[index+0];
			color_diff[1] += ((int)pixels[now_index+1]-(int)pixels[index+1])
				* ((int)pixels[now_index+1]-(int)pixels[index+0]);
			sum_color[1] += pixels[index+1];
			color_diff[2] += ((int)pixels[now_index+2]-(int)pixels[index+2])
				* ((int)pixels[now_index+2]-(int)pixels[index+0]);
			sum_color[2] += pixels[index+2];
			color_diff[3] += ((int)pixels[now_index+3]-(int)pixels[index+3])
				* ((int)pixels[now_index+3]-(int)pixels[index+3]);
			sum_color[3] += pixels[index+3];

			index = i*layer->stride + (j-1)*4;
			color_diff[0] += ((int)pixels[now_index+0]-(int)pixels[index+0])
				* ((int)pixels[now_index+0]-(int)pixels[index+0]);
			sum_color[0] += pixels[index+0];
			color_diff[1] += ((int)pixels[now_index+1]-(int)pixels[index+1])
				* ((int)pixels[now_index+1]-(int)pixels[index+0]);
			sum_color[1] += pixels[index+1];
			color_diff[2] += ((int)pixels[now_index+2]-(int)pixels[index+2])
				* ((int)pixels[now_index+2]-(int)pixels[index+0]);
			sum_color[2] += pixels[index+2];
			color_diff[3] += ((int)pixels[now_index+3]-(int)pixels[index+3])
				* ((int)pixels[now_index+3]-(int)pixels[index+3]);
			sum_color[3] += pixels[index+3];

			index += 8;
			color_diff[0] += ((int)pixels[now_index+0]-(int)pixels[index+0])
				* ((int)pixels[now_index+0]-(int)pixels[index+0]);
			sum_color[0] += pixels[index+0];
			color_diff[1] += ((int)pixels[now_index+1]-(int)pixels[index+1])
				* ((int)pixels[now_index+1]-(int)pixels[index+0]);
			sum_color[1] += pixels[index+1];
			color_diff[2] += ((int)pixels[now_index+2]-(int)pixels[index+2])
				* ((int)pixels[now_index+2]-(int)pixels[index+0]);
			sum_color[2] += pixels[index+2];
			color_diff[3] += ((int)pixels[now_index+3]-(int)pixels[index+3])
				* ((int)pixels[now_index+3]-(int)pixels[index+3]);
			sum_color[3] += pixels[index+3];

			index = (i+1)*layer->stride + (j-1)*4;
			color_diff[0] += ((int)pixels[now_index+0]-(int)pixels[index+0])
				* ((int)pixels[now_index+0]-(int)pixels[index+0]);
			sum_color[0] += pixels[index+0];
			color_diff[1] += ((int)pixels[now_index+1]-(int)pixels[index+1])
				* ((int)pixels[now_index+1]-(int)pixels[index+0]);
			sum_color[1] += pixels[index+1];
			color_diff[2] += ((int)pixels[now_index+2]-(int)pixels[index+2])
				* ((int)pixels[now_index+2]-(int)pixels[index+0]);
			sum_color[2] += pixels[index+2];
			color_diff[3] += ((int)pixels[now_index+3]-(int)pixels[index+3])
				* ((int)pixels[now_index+3]-(int)pixels[index+3]);
			sum_color[3] += pixels[index+3];

			index += 4;
			color_diff[0] += ((int)pixels[now_index+0]-(int)pixels[index+0])
				* ((int)pixels[now_index+0]-(int)pixels[index+0]);
			sum_color[0] += pixels[index+0];
			color_diff[1] += ((int)pixels[now_index+1]-(int)pixels[index+1])
				* ((int)pixels[now_index+1]-(int)pixels[index+0]);
			sum_color[1] += pixels[index+1];
			color_diff[2] += ((int)pixels[now_index+2]-(int)pixels[index+2])
				* ((int)pixels[now_index+2]-(int)pixels[index+0]);
			sum_color[2] += pixels[index+2];
			color_diff[3] += ((int)pixels[now_index+3]-(int)pixels[index+3])
				* ((int)pixels[now_index+3]-(int)pixels[index+3]);
			sum_color[3] += pixels[index+3];

			index += 4;
			color_diff[0] += ((int)pixels[now_index+0]-(int)pixels[index+0])
				* ((int)pixels[now_index+0]-(int)pixels[index+0]);
			sum_color[0] += pixels[index+0];
			color_diff[1] += ((int)pixels[now_index+1]-(int)pixels[index+1])
				* ((int)pixels[now_index+1]-(int)pixels[index+0]);
			sum_color[1] += pixels[index+1];
			color_diff[2] += ((int)pixels[now_index+2]-(int)pixels[index+2])
				* ((int)pixels[now_index+2]-(int)pixels[index+0]);
			sum_color[2] += pixels[index+2];
			color_diff[3] += ((int)pixels[now_index+3]-(int)pixels[index+3])
				* ((int)pixels[now_index+3]-(int)pixels[index+3]);
			sum_color[3] += pixels[index+3];

			sum_diff = color_diff[0] + color_diff[1] + color_diff[2] + color_diff[3];

			// �F���̍��v��臒l�ȏ�Ȃ�΃A���`�G�C���A�V���O���s
			if(sum_diff > ANTI_ALIAS_THRESHOLD)
			{
				temp->pixels[now_index+0] = (uint8)(sum_color[0] / 9);
				temp->pixels[now_index+1] = (uint8)(sum_color[1] / 9);
				temp->pixels[now_index+2] = (uint8)(sum_color[2] / 9);
				temp->pixels[now_index+3] = (uint8)(sum_color[3] / 9);
			}
			else
			{
				temp_pixels[now_index] = pixels[now_index];
				temp_pixels[now_index+1] = pixels[now_index+1];
				temp_pixels[now_index+2] = pixels[now_index+2];
				temp_pixels[now_index+3] = pixels[now_index+3];
			}
		}
	}	// 2�s�ڂ����ԉ���O�̍s�܂ŏ���
		// for(i=1; i<height-1; i++)
#ifdef _OPENMP
	omp_set_dynamic(TRUE);
	omp_set_num_threads(((APPLICATION*)(application))->max_threads);
#endif

	stride = (end_x - x - 2) * 4;
	// ���ʂ�Ԃ�
	for(i=y+1; i<end_y-1; i++)
	{
		(void)memcpy(&layer->pixels[i*layer->stride+(x+1)*4],
			&temp->pixels[i*layer->stride+(x+1)*4], stride);
	}

#undef ANTI_ALIAS_THRESHOLD
#undef MINIMUM_PARALLEL_SIZE
}

/*************************************************************************************
* AntiAliasLayerWithSelectionOrAlphaLock�֐�                                         *
* ���C���[�ɑ΂��Ĕ͈͂��w�肵�ăA���`�G�C���A�X��������(�I��͈͂܂��͓����x�ی�L) *
* layer			: �A���`�G�C���A�X�������郌�C���[                                   *
* out			: ���ʂ����郌�C���[                                               *
* selection		: �I��͈�                                                           *
* target_layer	: �A�N�e�B�u�ȃ��C���[                                               *
* rect			: �A���`�G�C���A�X��������͈�                                       *
* applicatoion	: �A�v���P�[�V�����S�̂��Ǘ�����\���̊Ǘ�����\����                 *
*************************************************************************************/
void AntiAliasLayerWithSelectionOrAlphaLock(
	LAYER *layer,
	LAYER* out,
	LAYER* selection,
	LAYER* target_layer,
	ANTI_ALIAS_RECTANGLE *rect,
	void* application
)
{
// �A���`�G�C���A�V���O���s��1�`�����l������臒l
#define ANTI_ALIAS_THRESHOLD ((51*51)/16)
#define MINIMUM_PARALLEL_SIZE 50
	// �A���`�G�C���A�X�J�n�E�I���̍��W
	int x, y, end_x, end_y;
	// ����1�s���̃o�C�g��
	int stride = layer->stride;
	// ��������s�N�Z��
	uint8 *pixels = layer->pixels;
	uint8 *out_pixels = out->pixels;
	int diff_red, diff_green, diff_blue, diff_alpha;
	int i;	// for���p�̃J�E���^

	// ���W�Ɣ͈͂�ݒ�
	end_x = rect->width;
	end_y = rect->height;
	if(rect->x < 0)
	{
		end_x += rect->x;
		x = 0;
	}
	else if(rect->x > layer->width)
	{
		return;
	}
	else
	{
		x = rect->x;
	}
	end_x = x + end_x;
	if(end_x > layer->width)
	{
		end_x = layer->width;
	}
	if(end_x - x < 3)
	{
		return;
	}

	if(rect->y < 0)
	{
		end_y += rect->y;
		y = 0;
	}
	else if(rect->y > layer->height)
	{
		return;
	}
	else
	{
		y = rect->y;
	}
	end_y = y + end_y;
	if(end_y > layer->height)
	{
		end_y = layer->height;
	}

	(void)memcpy(&out_pixels[y*layer->stride+x*4], &pixels[y*layer->stride+x*4],
		(end_x - x) * 4);

	// 2�s�ڂ����ԉ���O�̍s�܂ŏ���
#ifdef _OPENMP
	if(rect->height <= MINIMUM_PARALLEL_SIZE)
	{
		omp_set_dynamic(FALSE);
		omp_set_num_threads(1);
	}
#pragma omp parallel for firstprivate(pixels, out_pixels, stride, x, y, end_x)
#endif
	for(i=y+1; i<end_y-1; i++)
	{
		// �s�N�Z���f�[�^�z��̃C���f�b�N�X
		int now_index;
		int j;	// for���p�̃J�E���^

		// ��ԍ��͂��̂܂܃R�s�[
		now_index = i * stride + x * 4;
		out_pixels[now_index+0] = pixels[now_index+0];
		out_pixels[now_index+1] = pixels[now_index+1];
		out_pixels[now_index+2] = pixels[now_index+2];
		out_pixels[now_index+3] = pixels[now_index+3];

		now_index = i * layer->stride + (x+1)*4;

		for(j=x+1; j<end_x-1; j++, now_index+=4)
		{

			// �㉺���E�̃s�N�Z���̐F�����v�Z
			diff_red = ((int)pixels[now_index-4+0]-(int)pixels[now_index-stride+0])
				* ((int)pixels[now_index-4+0]-(int)pixels[now_index-stride+0]);
			diff_green = ((int)pixels[now_index-4+1]-(int)pixels[now_index-stride+1])
				* ((int)pixels[now_index-4+1]-(int)pixels[now_index-stride+1]);
			diff_blue = ((int)pixels[now_index-4+2]-(int)pixels[now_index-stride+2])
				* ((int)pixels[now_index-4+2]-(int)pixels[now_index-stride+2]);
			diff_alpha = ((int)pixels[now_index-4+3]-(int)pixels[now_index-stride+3])
				* ((int)pixels[now_index-4+3]-(int)pixels[now_index-stride+3]);
			if(diff_red + diff_green + diff_blue + diff_alpha > ANTI_ALIAS_THRESHOLD)
			{
				AntiAliasPixel(&pixels[now_index], &out_pixels[now_index], stride);
				continue;
			}

			diff_red = ((int)pixels[now_index+4+0]-(int)pixels[now_index-stride+0])
				* ((int)pixels[now_index+4+0]-(int)pixels[now_index-stride+0]);
			diff_green = ((int)pixels[now_index+4+1]-(int)pixels[now_index-stride+1])
				* ((int)pixels[now_index+4+1]-(int)pixels[now_index-stride+1]);
			diff_blue = ((int)pixels[now_index+4+2]-(int)pixels[now_index-stride+2])
				* ((int)pixels[now_index+4+2]-(int)pixels[now_index-stride+2]);
			diff_alpha = ((int)pixels[now_index+4+3]-(int)pixels[now_index-stride+3])
				* ((int)pixels[now_index+4+3]-(int)pixels[now_index-stride+3]);
			if(diff_red + diff_green + diff_blue + diff_alpha > ANTI_ALIAS_THRESHOLD)
			{
				AntiAliasPixel(&pixels[now_index], &out_pixels[now_index], stride);
				continue;
			}

			diff_red = ((int)pixels[now_index-4+0]-(int)pixels[now_index+stride+0])
				* ((int)pixels[now_index-4+0]-(int)pixels[now_index+stride+0]);
			diff_green = ((int)pixels[now_index-4+1]-(int)pixels[now_index+stride+1])
				* ((int)pixels[now_index-4+1]-(int)pixels[now_index+stride+1]);
			diff_blue = ((int)pixels[now_index-4+2]-(int)pixels[now_index+stride+2])
				* ((int)pixels[now_index-4+2]-(int)pixels[now_index+stride+2]);
			diff_alpha = ((int)pixels[now_index-4+3]-(int)pixels[now_index+stride+3])
				* ((int)pixels[now_index-4+3]-(int)pixels[now_index+stride+3]);
			if(diff_red + diff_green + diff_blue + diff_alpha > ANTI_ALIAS_THRESHOLD)
			{
				AntiAliasPixel(&pixels[now_index], &out_pixels[now_index], stride);
				continue;
			}

			diff_red = ((int)pixels[now_index+4+0]-(int)pixels[now_index+stride+0])
				* ((int)pixels[now_index+4+0]-(int)pixels[now_index+stride+0]);
			diff_green = ((int)pixels[now_index+4+1]-(int)pixels[now_index+stride+1])
				* ((int)pixels[now_index+4+1]-(int)pixels[now_index+stride+1]);
			diff_blue = ((int)pixels[now_index+4+2]-(int)pixels[now_index+stride+2])
				* ((int)pixels[now_index+4+2]-(int)pixels[now_index+stride+2]);
			diff_alpha = ((int)pixels[now_index+4+3]-(int)pixels[now_index+stride+3])
				* ((int)pixels[now_index+4+3]-(int)pixels[now_index+stride+3]);
			if(diff_red + diff_green + diff_blue + diff_alpha > ANTI_ALIAS_THRESHOLD)
			{
				AntiAliasPixel(&pixels[now_index], &out_pixels[now_index], stride);
				continue;
			}

			out_pixels[now_index+0] = pixels[now_index+0];
			out_pixels[now_index+1] = pixels[now_index+1];
			out_pixels[now_index+2] = pixels[now_index+2];
			out_pixels[now_index+3] = pixels[now_index+3];
		}
	}	// 2�s�ڂ����ԉ���O�̍s�܂ŏ���
		// for(i=1; i<height-1; i++)

	(void)memcpy(&out_pixels[(end_y-1)*layer->stride+x*4], &pixels[(end_y-1)*layer->stride+x*4],
		(end_x - x) * 4);

	// �I��͈͂ւ̑Ή�
	if(selection != NULL)
	{
#ifdef _OPENMP
# pragma omp parallel for firstprivate(out_pixels, rect, selection)
#endif
		for(i=0; i<rect->height; i++)
		{
			uint8 *selection_data = &selection->pixels[(i + rect->y) * selection->stride + rect->x];
			uint8 *buff = &out_pixels[(i + rect->y) * layer->stride + rect->x + 4];
			int j;
			for(j=0; j<rect->width; j++, selection_data++, buff+=4)
			{
				if(buff[3] > *selection_data)
				{
					buff[3] = *selection_data;
					if(buff[0] > buff[3])
					{
						buff[0] = buff[3];
					}
					if(buff[1] > buff[3])
					{
						buff[1] = buff[3];
					}
					if(buff[2] > buff[3])
					{
						buff[2] = buff[3];
					}
				}
			}
		}
	}

	// �����x�ی�ւ̑Ή�
	if(/*(target_layer->flags & LAYER_LOCK_OPACITY) != 0*/0)
	{
#ifdef _OPENMP
# pragma omp parallel for firstprivate(out_pixels, rect, target_layer)
#endif
		for(i=0; i<rect->height; i++)
		{
			uint8 *target = &target_layer->pixels[(i + rect->y) * target_layer->stride + rect->x * 4];
			uint8 *buff = &out_pixels[(i + rect->y) * layer->stride + rect->x * 4];
			int alpha;
			int j;
			for(j=0; j<rect->width; j++, target+=4, buff+=4)
			{
				if(target[3] < 255)
				{
					alpha = target[3] - buff[3];
					if(alpha >= 0)
					{
						target[3] = target[3] - alpha;
						if(target[0] > target[3])
						{
							target[0] = target[3];
						}
						if(target[1] > target[3])
						{
							target[1] = target[3];
						}
						if(target[2] > target[3])
						{
							target[2] = target[3];
						}

						if(buff[3] > alpha)
						{
							buff[3] = alpha;
						}
						if(buff[0] > buff[3])
						{
							buff[0] = buff[3];
						}
						if(buff[1] > buff[3])
						{
							buff[1] = buff[3];
						}
						if(buff[2] > buff[3])
						{
							buff[2] = buff[3];
						}
					}
					else
					{
						alpha = target[3];
						target[0] = 0;
						target[1] = 0;
						target[2] = 0;
						target[3] = 0;

						if(buff[3] > alpha)
						{
							buff[3] = alpha;
						}
						if(buff[0] > buff[3])
						{
							buff[0] = buff[3];
						}
						if(buff[1] > buff[3])
						{
							buff[1] = buff[3];
						}
						if(buff[2] > buff[3])
						{
							buff[2] = buff[3];
						}
					}
				}
			}
		}
	}

#ifdef _OPENMP
	omp_set_dynamic(TRUE);
	omp_set_num_threads(((APPLICATION*)(application))->max_threads);
#endif

#undef ANTI_ALIAS_THRESHOLD
#undef MINIMUM_PARALLEL_SIZE
}

/*************************************************************************************
* OldAntiAliasLayerWithSelectionOrAlphaLock�֐�                                      *
* ���C���[�ɑ΂��Ĕ͈͂��w�肵�ăA���`�G�C���A�X��������(�I��͈͂܂��͓����x�ی�L) *
* layer			: �A���`�G�C���A�X�������郌�C���[                                   *
* selection		: �I��͈�                                                           *
* target_layer	: �A�N�e�B�u�ȃ��C���[                                               *
* rect			: �A���`�G�C���A�X��������͈�                                       *
* applicatoion	: �A�v���P�[�V�����S�̂��Ǘ�����\���̊Ǘ�����\����                 *
*************************************************************************************/
void OldAntiAliasLayerWithSelectionOrAlphaLock(
	LAYER *layer,
	LAYER* temp,
	LAYER* selection,
	LAYER* target_layer,
	ANTI_ALIAS_RECTANGLE *rect,
	void* application
)
{
// �A���`�G�C���A�V���O���s��1�`�����l������臒l
#define ANTI_ALIAS_THRESHOLD (51*51)
#define MINIMUM_PARALLEL_SIZE 50
	// �A���`�G�C���A�X�J�n�E�I���̍��W
	int x, y, end_x, end_y;
	// ����1�s���̃o�C�g��
	int stride;
	// �������郌�C���[�̈�s���̃o�C�g��
	int layer_stride = layer->stride;
	// ��������s�N�Z��
	uint8 *pixels = layer->pixels;
	uint8 *temp_pixels = temp->pixels;
	int i;	// for���p�̃J�E���^

	// ���W�Ɣ͈͂�ݒ�
	end_x = rect->width;
	end_y = rect->height;
	if(rect->x < 0)
	{
		end_x += rect->x;
		x = 0;
	}
	else if(rect->x > layer->width)
	{
		return;
	}
	else
	{
		x = rect->x;
	}
	end_x = x + end_x;
	if(end_x > layer->width)
	{
		end_x = layer->width;
	}
	if(end_x - x < 3)
	{
		return;
	}

	if(rect->y < 0)
	{
		end_y += rect->y;
		y = 0;
	}
	else if(rect->y > layer->height)
	{
		return;
	}
	else
	{
		y = rect->y;
	}
	end_y = y + end_y;
	if(end_y > layer->height)
	{
		end_y = layer->height;
	}

	// 2�s�ڂ����ԉ���O�̍s�܂ŏ���
#ifdef _OPENMP
	if(rect->height <= MINIMUM_PARALLEL_SIZE)
	{
		omp_set_dynamic(FALSE);
		omp_set_num_threads(1);
	}
#pragma omp parallel for firstprivate(pixels, temp_pixels, layer_stride, x, y, end_x)
#endif
	for(i=y+1; i<end_y-1; i++)
	{
		// �s�N�Z���f�[�^�z��̃C���f�b�N�X
		int index, now_index;
		// ���݂̃s�N�Z���Ǝ��͂̃s�N�Z���̐F��
		int color_diff[4];
		// �F���̍��v
		int sum_diff;
		// ���͂̃s�N�Z���̍��v�l
		int sum_color[4];
		int j;	// for���p�̃J�E���^

		// ��ԍ��͂��̂܂܃R�s�[
		now_index = i * layer->stride + (x+1)*4;

		for(j=x+1; j<end_x-1; j++, now_index+=4)
		{

			// ����8�s�N�Z���Ƃ̐F���ƒl�̍��v���v�Z
			index = (i-1)*layer_stride + (j-1)*4;
			color_diff[0] = ((int)pixels[now_index+0]-(int)pixels[index+0])
				* ((int)pixels[now_index+0]-(int)pixels[index+0]);
			sum_color[0] = pixels[now_index+0];
			sum_color[0] += pixels[index+0];
			color_diff[1] = ((int)pixels[now_index+1]-(int)pixels[index+1])
				* ((int)pixels[now_index+1]-(int)pixels[index+0]);
			sum_color[1] = pixels[now_index+1];
			sum_color[1] += pixels[index+1];
			color_diff[2] = ((int)pixels[now_index+2]-(int)pixels[index+2])
				* ((int)pixels[now_index+2]-(int)pixels[index+0]);
			sum_color[2] = pixels[now_index+2];
			sum_color[2] += pixels[index+2];
			color_diff[3] = ((int)pixels[now_index+3]-(int)pixels[index+3])
				* ((int)pixels[now_index+3]-(int)pixels[index+3]);
			sum_color[3] = pixels[now_index+3];
			sum_color[3] += pixels[index+3];

			index += 4;
			color_diff[0] += ((int)pixels[now_index+0]-(int)pixels[index+0])
				* ((int)pixels[now_index+0]-(int)pixels[index+0]);
			sum_color[0] += pixels[index+0];
			color_diff[1] += ((int)pixels[now_index+1]-(int)pixels[index+1])
				* ((int)pixels[now_index+1]-(int)pixels[index+0]);
			sum_color[1] += pixels[index+1];
			color_diff[2] += ((int)pixels[now_index+2]-(int)pixels[index+2])
				* ((int)pixels[now_index+2]-(int)pixels[index+0]);
			sum_color[2] += pixels[index+2];
			color_diff[3] += ((int)pixels[now_index+3]-(int)pixels[index+3])
				* ((int)pixels[now_index+3]-(int)pixels[index+3]);
			sum_color[3] += pixels[index+3];

			index += 4;
			color_diff[0] += ((int)pixels[now_index+0]-(int)pixels[index+0])
				* ((int)pixels[now_index+0]-(int)pixels[index+0]);
			sum_color[0] += pixels[index+0];
			color_diff[1] += ((int)pixels[now_index+1]-(int)pixels[index+1])
				* ((int)pixels[now_index+1]-(int)pixels[index+0]);
			sum_color[1] += pixels[index+1];
			color_diff[2] += ((int)pixels[now_index+2]-(int)pixels[index+2])
				* ((int)pixels[now_index+2]-(int)pixels[index+0]);
			sum_color[2] += pixels[index+2];
			color_diff[3] += ((int)pixels[now_index+3]-(int)pixels[index+3])
				* ((int)pixels[now_index+3]-(int)pixels[index+3]);
			sum_color[3] += pixels[index+3];

			index = i*layer->stride + (j-1)*4;
			color_diff[0] += ((int)pixels[now_index+0]-(int)pixels[index+0])
				* ((int)pixels[now_index+0]-(int)pixels[index+0]);
			sum_color[0] += pixels[index+0];
			color_diff[1] += ((int)pixels[now_index+1]-(int)pixels[index+1])
				* ((int)pixels[now_index+1]-(int)pixels[index+0]);
			sum_color[1] += pixels[index+1];
			color_diff[2] += ((int)pixels[now_index+2]-(int)pixels[index+2])
				* ((int)pixels[now_index+2]-(int)pixels[index+0]);
			sum_color[2] += pixels[index+2];
			color_diff[3] += ((int)pixels[now_index+3]-(int)pixels[index+3])
				* ((int)pixels[now_index+3]-(int)pixels[index+3]);
			sum_color[3] += pixels[index+3];

			index += 8;
			color_diff[0] += ((int)pixels[now_index+0]-(int)pixels[index+0])
				* ((int)pixels[now_index+0]-(int)pixels[index+0]);
			sum_color[0] += pixels[index+0];
			color_diff[1] += ((int)pixels[now_index+1]-(int)pixels[index+1])
				* ((int)pixels[now_index+1]-(int)pixels[index+0]);
			sum_color[1] += pixels[index+1];
			color_diff[2] += ((int)pixels[now_index+2]-(int)pixels[index+2])
				* ((int)pixels[now_index+2]-(int)pixels[index+0]);
			sum_color[2] += pixels[index+2];
			color_diff[3] += ((int)pixels[now_index+3]-(int)pixels[index+3])
				* ((int)pixels[now_index+3]-(int)pixels[index+3]);
			sum_color[3] += pixels[index+3];

			index = (i+1)*layer->stride + (j-1)*4;
			color_diff[0] += ((int)pixels[now_index+0]-(int)pixels[index+0])
				* ((int)pixels[now_index+0]-(int)pixels[index+0]);
			sum_color[0] += pixels[index+0];
			color_diff[1] += ((int)pixels[now_index+1]-(int)pixels[index+1])
				* ((int)pixels[now_index+1]-(int)pixels[index+0]);
			sum_color[1] += pixels[index+1];
			color_diff[2] += ((int)pixels[now_index+2]-(int)pixels[index+2])
				* ((int)pixels[now_index+2]-(int)pixels[index+0]);
			sum_color[2] += pixels[index+2];
			color_diff[3] += ((int)pixels[now_index+3]-(int)pixels[index+3])
				* ((int)pixels[now_index+3]-(int)pixels[index+3]);
			sum_color[3] += pixels[index+3];

			index += 4;
			color_diff[0] += ((int)pixels[now_index+0]-(int)pixels[index+0])
				* ((int)pixels[now_index+0]-(int)pixels[index+0]);
			sum_color[0] += pixels[index+0];
			color_diff[1] += ((int)pixels[now_index+1]-(int)pixels[index+1])
				* ((int)pixels[now_index+1]-(int)pixels[index+0]);
			sum_color[1] += pixels[index+1];
			color_diff[2] += ((int)pixels[now_index+2]-(int)pixels[index+2])
				* ((int)pixels[now_index+2]-(int)pixels[index+0]);
			sum_color[2] += pixels[index+2];
			color_diff[3] += ((int)pixels[now_index+3]-(int)pixels[index+3])
				* ((int)pixels[now_index+3]-(int)pixels[index+3]);
			sum_color[3] += pixels[index+3];

			index += 4;
			color_diff[0] += ((int)pixels[now_index+0]-(int)pixels[index+0])
				* ((int)pixels[now_index+0]-(int)pixels[index+0]);
			sum_color[0] += pixels[index+0];
			color_diff[1] += ((int)pixels[now_index+1]-(int)pixels[index+1])
				* ((int)pixels[now_index+1]-(int)pixels[index+0]);
			sum_color[1] += pixels[index+1];
			color_diff[2] += ((int)pixels[now_index+2]-(int)pixels[index+2])
				* ((int)pixels[now_index+2]-(int)pixels[index+0]);
			sum_color[2] += pixels[index+2];
			color_diff[3] += ((int)pixels[now_index+3]-(int)pixels[index+3])
				* ((int)pixels[now_index+3]-(int)pixels[index+3]);
			sum_color[3] += pixels[index+3];

			sum_diff = color_diff[0] + color_diff[1] + color_diff[2] + color_diff[3];

			// �F���̍��v��臒l�ȏ�Ȃ�΃A���`�G�C���A�V���O���s
			if(sum_diff > ANTI_ALIAS_THRESHOLD)
			{
				temp->pixels[now_index+0] = (uint8)(sum_color[0] / 9);
				temp->pixels[now_index+1] = (uint8)(sum_color[1] / 9);
				temp->pixels[now_index+2] = (uint8)(sum_color[2] / 9);
				temp->pixels[now_index+3] = (uint8)(sum_color[3] / 9);
			}
			else
			{
				temp_pixels[now_index] = pixels[now_index];
				temp_pixels[now_index+1] = pixels[now_index+1];
				temp_pixels[now_index+2] = pixels[now_index+2];
				temp_pixels[now_index+3] = pixels[now_index+3];
			}
		}
	}	// 2�s�ڂ����ԉ���O�̍s�܂ŏ���
		// for(i=1; i<height-1; i++)

	// �I��͈͂ւ̑Ή�
	if(selection != NULL)
	{
#ifdef _OPENMP
# pragma omp parallel for firstprivate(pixels, rect, selection)
#endif
		for(i=0; i<rect->height; i++)
		{
			uint8 *selection_data = &selection->pixels[(i + rect->y) * selection->stride + rect->x];
			uint8 *buff = &pixels[(i + rect->y) * layer->stride + rect->x + 4];
			int j;
			for(j=0; j<rect->width; j++, selection_data++, buff+=4)
			{
				if(buff[3] > *selection_data)
				{
					buff[3] = *selection_data;
					if(buff[0] > buff[3])
					{
						buff[0] = buff[3];
					}
					if(buff[1] > buff[3])
					{
						buff[1] = buff[3];
					}
					if(buff[2] > buff[3])
					{
						buff[2] = buff[3];
					}
				}
			}
		}
	}

	// �����x�ی�ւ̑Ή�
	if(/*(target_layer->flags & LAYER_LOCK_OPACITY) != 0*/0)
	{
#ifdef _OPENMP
# pragma omp parallel for firstprivate(pixels, rect, target_layer)
#endif
		for(i=0; i<rect->height; i++)
		{
			uint8 *target = &target_layer->pixels[(i + rect->y) * target_layer->stride + rect->x * 4];
			uint8 *buff = &pixels[(i + rect->y) * layer->stride + rect->x * 4];
			int alpha;
			int j;
			for(j=0; j<rect->width; j++, target+=4, buff+=4)
			{
				if(target[3] < 255)
				{
					alpha = target[3] - buff[3];
					if(alpha >= 0)
					{
						target[3] = target[3] - alpha;
						if(target[0] > target[3])
						{
							target[0] = target[3];
						}
						if(target[1] > target[3])
						{
							target[1] = target[3];
						}
						if(target[2] > target[3])
						{
							target[2] = target[3];
						}

						if(buff[3] > alpha)
						{
							buff[3] = alpha;
						}
						if(buff[0] > buff[3])
						{
							buff[0] = buff[3];
						}
						if(buff[1] > buff[3])
						{
							buff[1] = buff[3];
						}
						if(buff[2] > buff[3])
						{
							buff[2] = buff[3];
						}
					}
					else
					{
						alpha = target[3];
						target[0] = 0;
						target[1] = 0;
						target[2] = 0;
						target[3] = 0;

						if(buff[3] > alpha)
						{
							buff[3] = alpha;
						}
						if(buff[0] > buff[3])
						{
							buff[0] = buff[3];
						}
						if(buff[1] > buff[3])
						{
							buff[1] = buff[3];
						}
						if(buff[2] > buff[3])
						{
							buff[2] = buff[3];
						}
					}
				}
			}
		}
	}

#ifdef _OPENMP
	omp_set_dynamic(TRUE);
	omp_set_num_threads(((APPLICATION*)(application))->max_threads);
#endif

	stride = (end_x - x - 2) * 4;
	// ���ʂ�Ԃ�
	for(i=y+1; i<end_y-1; i++)
	{
		(void)memcpy(&layer->pixels[i*layer->stride+(x+1)*4],
			&temp->pixels[i*layer->stride+(x+1)*4], stride);
	}
}

/*********************************************************************
* AntiAliasVectorLine�֐�                                            *
* �x�N�g�����C���[�̐��ɑ΂��Ĕ͈͂��w�肵�ăA���`�G�C���A�X�������� *
* layer	: �A���`�G�C���A�X�������郌�C���[                           *
* rect	: �A���`�G�C���A�X��������͈�                               *
*********************************************************************/
void AntiAliasVectorLine(LAYER *layer, LAYER* temp, ANTI_ALIAS_RECTANGLE *rect)
{
// �A���`�G�C���A�V���O���s��1�`�����l������臒l
#define ANTI_ALIAS_THRESHOLD (51*51)
	// �A���`�G�C���A�X�J�n�E�I���̍��W
	int x, y, end_x, end_y;
	// ����1�s���̃o�C�g��
	int stride;
	// �������郌�C���[�̈�s���̃o�C�g��
	int layer_stride = layer->stride;
	// ��������s�N�Z��
	uint8 *pixels = layer->pixels;
	uint8 *temp_pixels = temp->pixels;
	int i;	// for���p�̃J�E���^

	// ���W�Ɣ͈͂�ݒ�
	end_x = rect->width;
	end_y = rect->height;
	if(rect->x < 0)
	{
		end_x += rect->x;
		x = 0;
	}
	else if(rect->x > layer->width)
	{
		return;
	}
	else
	{
		x = rect->x;
	}
	end_x = x + end_x;
	if(end_x > layer->width)
	{
		end_x = layer->width;
	}
	if(end_x - x < 3)
	{
		return;
	}

	if(rect->y < 0)
	{
		end_y += rect->y;
		y = 0;
	}
	else if(rect->y > layer->height)
	{
		return;
	}
	else
	{
		y = rect->y;
	}
	end_y = y + end_y;
	if(end_y > layer->height)
	{
		end_y = layer->height;
	}

	// 2�s�ڂ����ԉ���O�̍s�܂ŏ���
#ifdef _OPENMP
#pragma omp parallel for firstprivate(pixels, temp_pixels, layer_stride, x, y, end_x)
#endif
	for(i=y+1; i<end_y-1; i++)
	{
		// �s�N�Z���f�[�^�z��̃C���f�b�N�X
		int index, now_index;
		// ���݂̃s�N�Z���Ǝ��͂̃s�N�Z���̐F��
		int color_diff[4];
		// �F���̍��v
		int sum_diff;
		// ���͂̃s�N�Z���̍��v�l
		int sum_color[4];
		int j;	// for���p�̃J�E���^

		// ��ԍ��͂��̂܂܃R�s�[
		now_index = i * layer->stride + (x+1)*4;

		for(j=x+1; j<end_x-1; j++, now_index+=4)
		{

			// ����8�s�N�Z���Ƃ̐F���ƒl�̍��v���v�Z
			index = (i-1)*layer_stride + (j-1)*4;
			color_diff[0] = ((int)pixels[now_index+0]-(int)pixels[index+0])
				* ((int)pixels[now_index+0]-(int)pixels[index+0]);
			sum_color[0] = pixels[now_index+0];
			sum_color[0] += pixels[index+0];
			color_diff[1] = ((int)pixels[now_index+1]-(int)pixels[index+1])
				* ((int)pixels[now_index+1]-(int)pixels[index+0]);
			sum_color[1] = pixels[now_index+1];
			sum_color[1] += pixels[index+1];
			color_diff[2] = ((int)pixels[now_index+2]-(int)pixels[index+2])
				* ((int)pixels[now_index+2]-(int)pixels[index+0]);
			sum_color[2] = pixels[now_index+2];
			sum_color[2] += pixels[index+2];
			color_diff[3] = ((int)pixels[now_index+3]-(int)pixels[index+3])
				* ((int)pixels[now_index+3]-(int)pixels[index+3]);
			sum_color[3] = pixels[now_index+3];
			sum_color[3] += pixels[index+3];

			index += 4;
			color_diff[0] += ((int)pixels[now_index+0]-(int)pixels[index+0])
				* ((int)pixels[now_index+0]-(int)pixels[index+0]);
			sum_color[0] += pixels[index+0];
			color_diff[1] += ((int)pixels[now_index+1]-(int)pixels[index+1])
				* ((int)pixels[now_index+1]-(int)pixels[index+0]);
			sum_color[1] += pixels[index+1];
			color_diff[2] += ((int)pixels[now_index+2]-(int)pixels[index+2])
				* ((int)pixels[now_index+2]-(int)pixels[index+0]);
			sum_color[2] += pixels[index+2];
			color_diff[3] += ((int)pixels[now_index+3]-(int)pixels[index+3])
				* ((int)pixels[now_index+3]-(int)pixels[index+3]);
			sum_color[3] += pixels[index+3];

			index += 4;
			color_diff[0] += ((int)pixels[now_index+0]-(int)pixels[index+0])
				* ((int)pixels[now_index+0]-(int)pixels[index+0]);
			sum_color[0] += pixels[index+0];
			color_diff[1] += ((int)pixels[now_index+1]-(int)pixels[index+1])
				* ((int)pixels[now_index+1]-(int)pixels[index+0]);
			sum_color[1] += pixels[index+1];
			color_diff[2] += ((int)pixels[now_index+2]-(int)pixels[index+2])
				* ((int)pixels[now_index+2]-(int)pixels[index+0]);
			sum_color[2] += pixels[index+2];
			color_diff[3] += ((int)pixels[now_index+3]-(int)pixels[index+3])
				* ((int)pixels[now_index+3]-(int)pixels[index+3]);
			sum_color[3] += pixels[index+3];

			index = i*layer->stride + (j-1)*4;
			color_diff[0] += ((int)pixels[now_index+0]-(int)pixels[index+0])
				* ((int)pixels[now_index+0]-(int)pixels[index+0]);
			sum_color[0] += pixels[index+0];
			color_diff[1] += ((int)pixels[now_index+1]-(int)pixels[index+1])
				* ((int)pixels[now_index+1]-(int)pixels[index+0]);
			sum_color[1] += pixels[index+1];
			color_diff[2] += ((int)pixels[now_index+2]-(int)pixels[index+2])
				* ((int)pixels[now_index+2]-(int)pixels[index+0]);
			sum_color[2] += pixels[index+2];
			color_diff[3] += ((int)pixels[now_index+3]-(int)pixels[index+3])
				* ((int)pixels[now_index+3]-(int)pixels[index+3]);
			sum_color[3] += pixels[index+3];

			index += 8;
			color_diff[0] += ((int)pixels[now_index+0]-(int)pixels[index+0])
				* ((int)pixels[now_index+0]-(int)pixels[index+0]);
			sum_color[0] += pixels[index+0];
			color_diff[1] += ((int)pixels[now_index+1]-(int)pixels[index+1])
				* ((int)pixels[now_index+1]-(int)pixels[index+0]);
			sum_color[1] += pixels[index+1];
			color_diff[2] += ((int)pixels[now_index+2]-(int)pixels[index+2])
				* ((int)pixels[now_index+2]-(int)pixels[index+0]);
			sum_color[2] += pixels[index+2];
			color_diff[3] += ((int)pixels[now_index+3]-(int)pixels[index+3])
				* ((int)pixels[now_index+3]-(int)pixels[index+3]);
			sum_color[3] += pixels[index+3];

			index = (i+1)*layer->stride + (j-1)*4;
			color_diff[0] += ((int)pixels[now_index+0]-(int)pixels[index+0])
				* ((int)pixels[now_index+0]-(int)pixels[index+0]);
			sum_color[0] += pixels[index+0];
			color_diff[1] += ((int)pixels[now_index+1]-(int)pixels[index+1])
				* ((int)pixels[now_index+1]-(int)pixels[index+0]);
			sum_color[1] += pixels[index+1];
			color_diff[2] += ((int)pixels[now_index+2]-(int)pixels[index+2])
				* ((int)pixels[now_index+2]-(int)pixels[index+0]);
			sum_color[2] += pixels[index+2];
			color_diff[3] += ((int)pixels[now_index+3]-(int)pixels[index+3])
				* ((int)pixels[now_index+3]-(int)pixels[index+3]);
			sum_color[3] += pixels[index+3];

			index += 4;
			color_diff[0] += ((int)pixels[now_index+0]-(int)pixels[index+0])
				* ((int)pixels[now_index+0]-(int)pixels[index+0]);
			sum_color[0] += pixels[index+0];
			color_diff[1] += ((int)pixels[now_index+1]-(int)pixels[index+1])
				* ((int)pixels[now_index+1]-(int)pixels[index+0]);
			sum_color[1] += pixels[index+1];
			color_diff[2] += ((int)pixels[now_index+2]-(int)pixels[index+2])
				* ((int)pixels[now_index+2]-(int)pixels[index+0]);
			sum_color[2] += pixels[index+2];
			color_diff[3] += ((int)pixels[now_index+3]-(int)pixels[index+3])
				* ((int)pixels[now_index+3]-(int)pixels[index+3]);
			sum_color[3] += pixels[index+3];

			index += 4;
			color_diff[0] += ((int)pixels[now_index+0]-(int)pixels[index+0])
				* ((int)pixels[now_index+0]-(int)pixels[index+0]);
			sum_color[0] += pixels[index+0];
			color_diff[1] += ((int)pixels[now_index+1]-(int)pixels[index+1])
				* ((int)pixels[now_index+1]-(int)pixels[index+0]);
			sum_color[1] += pixels[index+1];
			color_diff[2] += ((int)pixels[now_index+2]-(int)pixels[index+2])
				* ((int)pixels[now_index+2]-(int)pixels[index+0]);
			sum_color[2] += pixels[index+2];
			color_diff[3] += ((int)pixels[now_index+3]-(int)pixels[index+3])
				* ((int)pixels[now_index+3]-(int)pixels[index+3]);
			sum_color[3] += pixels[index+3];

			sum_diff = color_diff[0] + color_diff[1] + color_diff[2] + color_diff[3];

			// �F���̍��v��臒l�ȏ�Ȃ�΃A���`�G�C���A�V���O���s
			if(sum_diff > ANTI_ALIAS_THRESHOLD)
			{	// �ݒ肷��V���ȃf�[�^
				uint8 new_value;

				new_value = (uint8)(sum_color[0] / 9);
				temp->pixels[now_index+0] = new_value;

				new_value = (uint8)(sum_color[1] / 9);
				temp->pixels[now_index+1] = new_value;
				
				new_value = (uint8)(sum_color[2] / 9);
				temp->pixels[now_index+2] = new_value;

				new_value = (uint8)(sum_color[3] / 9);
				temp->pixels[now_index+3] = new_value;
			}
			else
			{
				temp_pixels[now_index] = pixels[now_index];
				temp_pixels[now_index+1] = pixels[now_index+1];
				temp_pixels[now_index+2] = pixels[now_index+2];
				temp_pixels[now_index+3] = pixels[now_index+3];
			}
		}
	}	// 2�s�ڂ����ԉ���O�̍s�܂ŏ���
		// for(i=1; i<height-1; i++)

	stride = (end_x - x - 2) * 4;
	// ���ʂ�Ԃ�
	for(i=y+1; i<end_y-1; i++)
	{
		(void)memcpy(&layer->pixels[i*layer->stride+(x+1)*4],
			&temp->pixels[i*layer->stride+(x+1)*4], stride);
	}
}

#ifdef __cplusplus
}
#endif
