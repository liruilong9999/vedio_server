1.����
   live_ex�ڿ�Դ��ý�������live555�Ļ����ϣ���չ�˶�avi��mp4�����ļ���ʽ��֧�֣�ý������ʽ֧��mp3��aac��mpeg4��h264���ļ���ʽ�Ľ�����ffmpeg��ʵ�֡�

2.ע��
1).����ʹ�õ�live555�汾Ϊlive.2011.11.20��
2).���Է��֣�ʹ��vlc����avi����mp4�е�mpeg4��Ƶʱ���ǳ�����cpu��ﵽ100%������ʹ��ffplay��mplayerʱ����������
3).����h264��Ƶ��ʽ,���ʽϴ�ʱ��������Ҫ����SEI����Ĭ�ϵ���󳤶ȣ�����
��livemedia\H264VideoStreamFramer.cpp,�ҵ������У�
#define SEI_MAX_SIZE 5000 // larger than the largest possible SEI NAL unit
�������ʱ������ֵ�޸�Ϊ10000��

2016-7-12
1.ȥ����ʱ���Ļ�ȡ��֧��vlc��ͣ�󲥷�

2016-6-12
1.����֡�ʵĻ�ȡ����ȡ�ۺ�֡��

2016-5-25
1.�޸�MTU���ֵΪ1412 MultiFramedRTPSink.cpp MultiFramedRTPSink 47�� setPacketSizes();

2.����֡�ʵĴ��ݵ�FrameSource H264or5VideoStreamFramer.cpp 88��;

3.����֡�ʵĻ�ȡ ͨ�� avg_frame_rate.num/(double)avg_frame_rate.den��ȡ�ۺ�֡��

2016-4-21
�����汾 ע������:
1.���л���Ϊ��Ŀ¼�����У���������ȷ���㲥·����ȷ��
2.�˿�Ϊ554��8554����ͬ�����¿�����554��8554��Ĭ����554��554��������Ϊ8554��

