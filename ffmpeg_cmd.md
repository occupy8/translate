#添加静音音频

ffmpeg -f lavfi -i anullsrc=channel_layout=mono:sample_rate=48000 -i 1.ts -shortest -c:v copy -c:a aac output.ts

channel_layout=mono:sample_rate=48000

#分离音视频
ffmpeg -i 3.ts -map 0:0 -c copy output_0.ts   //把stream 0分离出来

#生成黑色视频
ffmpeg -ss 0 -t 30 -f lavfi -i color=c=0x00000:s=1280*720:r=25 -vcodec libx264 -r:v 25 test.ts  
0x000000  为配置颜色的RGB值   -t 配置输出视频时长  -r 帧率  -s 分辨率

#合并音视频
ffmpeg -i v.ts -i a.ts -c:v copy -c:a aac -strict experimental av.ts

#图片转换为yuv图
ffmpeg -i test.jpg -pix_fmt yuv420p  test.yuv

#查看yuv图片
ffplay [-f rawvideo] -video_size  640*480 test.yuv
