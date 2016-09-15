 
bcm        = 'GRBG'; 
blk = 64;
gain = 1; % 你看亮度自己设增益， 

filename = '../data/raw_2016x1504_Out.raw';

RawWid = 2016;
RawHgt = 1504;
fid = fopen(filename, 'rb');
RAW = fread(fid,[RawWid, RawHgt],'uint16');
% RAW = fread(fid,[RawWid, RawHgt],'ubit10');
RAW = RAW';       
fclose(fid);
% RAW = RAW([2:end,1],:);
WriteRaw2RGB_Demosaic(RAW, bcm, blk, gain, filename)

