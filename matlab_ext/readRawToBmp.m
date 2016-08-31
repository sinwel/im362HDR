 
bcm        = 'RGGB'; 
blk = 64;
gain = 1; % 你看亮度自己设增益， 
% filename = '../data/test317/test317-4164_3136_v241rkTest01837010611_16.raw';
% filename = '../data/test01/test01-4164_3136_v327rkTest01307010612_10.raw';
filename = '../data/test317/test317-4164_3136_v327rkTest01307010612_10.raw';
% filename = '../data/test01/test01-4164_3136_0_16.raw';
RawWid = 4164;
RawHgt = 3136;
fid = fopen(filename, 'rb');
% RAW = fread(fid,[RawWid, RawHgt],'uint16');
RAW = fread(fid,[RawWid, RawHgt],'ubit10');
RAW = RAW';       
fclose(fid);
% RAW = RAW([2:end,1],:);
WriteRaw2RGB_Demosaic(RAW, bcm, blk, gain, filename)

