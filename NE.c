#include <stdint.h>
#include <stdio.h>
#include "x264.h"
#include <stdlib.h>




int my_rand(){
    return rand()%81 + 50;//产生60~140的随机数！！
}
//X264_API int x264_encoder_reconfig( x264_t *, x264_param_t * );
void  my_Adjust_Rate(x264_t * h, x264_param_t *  param){
        int  my_i_bitrate = my_rand();
        printf("预设的平均码率值是:%d\n",my_i_bitrate);
        param->rc.i_bitrate =  my_i_bitrate;
        param->rc.i_vbv_max_bitrate = my_i_bitrate;
        //param->rc.i_vbv_buffer_size = my_i_bitrate;
        int re = x264_encoder_reconfig(h, param);
        if (re == 0)
        {
            //h = x264_encoder_open( param );
            printf("设置生效！！\n");
            printf("实际平均码率值:%d\n",param->rc.i_bitrate);
        }else
        {
            printf("设置失败！！");
        }
}

int main( int argc, char **argv )
{
    int width = 640;
    int height= 360;
    x264_param_t param;
    x264_picture_t pic;
    x264_picture_t pic_out;
    x264_t *h;
    int i_frame = 0;
    int i_frame_size;
    x264_nal_t *nal;
    int i_nal;

    FILE* fp_src  = fopen("demo_640x360_yuv420p.yuv", "rb");
	FILE* fp_dst = fopen("out.h264", "wb");

    /* Get default params for preset/tuning */
    //preset 是编码速度，　tune是编码质量和画面细节相关参数！！
    if( x264_param_default_preset( &param, "medium", NULL ) < 0 ){
        return -1;
    }
        
    /* Configure non-default params */
    param.i_bitdepth = 8;
    param.i_csp = X264_CSP_I420;
    param.i_width  = width;
    param.i_height = height;
    param.b_vfr_input = 0;
    param.b_repeat_headers = 1; //put SPS/PPS before each keyframe
    param.b_annexb = 1;//决定NALU前４个字节，是用start codes(1)，还是size nalu(0).
    
    param.i_fps_num = 20;
    param.i_fps_den = 1;  //表示帧率的参数这个会写进文件里面，影戏的是播放的速率！

    // 图像的质量控制
    param.rc.f_rf_constant = 25; 
    param.rc.f_rf_constant_max = 45;


    //X264_API int x264_encoder_reconfig( x264_t *, x264_param_t * );

    //码率控制
    /**
     * 参数i_rc_method表示码率控制
     * CQP：恒定质量
     * CRF:恒定码率
     * ABR:平均码率。只有设置此值才能实际控制码流速率
     * 
     * rc:是一个关于码率控制的结构体。
    */
      param.rc.i_rc_method = X264_RC_ABR; 
      param.rc.i_bitrate =70;//单位：Kbps(K bit per second)  //设置平均码率
      param.rc.i_vbv_max_bitrate = 70;     //平均码率模式下的，最大瞬时码率
      param.rc.f_rate_tolerance = 1; //平均码率的模式下，瞬时码率可以偏离的倍数，范围是0.1 ~100,默认是1.0
      
      param.rc.i_vbv_buffer_size = 70; //调整的时候可以没有这个参数，初始化的时候必须有，否则无法起到调整码率的作用！！
    /* Apply profile restrictions. */

    

    if( x264_param_apply_profile( &param, "baseline" ) < 0 ){
        return -1;
    }

    if( x264_picture_alloc( &pic, param.i_csp, param.i_width, param.i_height ) < 0 ){
        return -1;
    }
    
    
  

    h = x264_encoder_open( &param );
    if( !h ){   //打开错误的是要才进行清理！
        x264_picture_clean( &pic );
    }

    int luma_size = width * height;
    int chroma_size = luma_size / 4;
    /* Encode frames */
    for( ;; i_frame++ )
    {

        my_Adjust_Rate(h, &param);

        if( fread( pic.img.plane[0], 1, luma_size, fp_src ) != luma_size ){
            break;//读取Y分量
        }
            
        if( fread( pic.img.plane[1], 1, chroma_size, fp_src ) != chroma_size ){
            break;//读取U分量
        }
            
        if( fread( pic.img.plane[2], 1, chroma_size, fp_src ) != chroma_size ){
            break;//读取V分量
        }
            

        pic.i_pts = i_frame;
        //returns the number of bytes in the returned NALs.
        i_frame_size = x264_encoder_encode( h, &nal, &i_nal, &pic, &pic_out );
        if( i_frame_size < 0 ){ //返回值小于０，代表了错误！！
            x264_encoder_close(h);
        }
        else if( i_frame_size )  //返回值为０代表没有nalu的输出，返回值大于０代表输出的nalu的大小！
        {
            printf("\n---------------进入编码的输出----------------\n");
            if( !fwrite( nal->p_payload, i_frame_size, 1, fp_dst ) ){
                x264_encoder_close( h );
            }
        }
    }
    /* Flush delayed frames */
    while( x264_encoder_delayed_frames( h ) )
    {
        i_frame_size = x264_encoder_encode( h, &nal, &i_nal, NULL, &pic_out );
        if( i_frame_size < 0 ){
            x264_encoder_close( h );
        }
        else if( i_frame_size )
        {
            if( !fwrite( nal->p_payload, i_frame_size, 1, fp_dst ) ){
                x264_encoder_close( h );
            }
        }
    }

    x264_encoder_close( h );
    x264_picture_clean( &pic );

    fclose(fp_src);
    fclose(fp_dst);

    return 0;
}
