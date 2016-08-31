function WriteRaw2RGB_Demosaic(RAW, bcm, blk, gain, filename)


%%

%{1
gains = ones(4,1);
gains(strfind(bcm,'R')) = 1.8;
gains(strfind(bcm,'B')) = 2.2;

%
RAW2 = max(RAW - blk, 0);
%{
blks = [261 262 259 261];
RAW2(1:2:end, 1:2:end) = RAW2(1:2:end, 1:2:end) - blks(1);
RAW2(1:2:end, 2:2:end) = RAW2(1:2:end, 2:2:end) - blks(2);
RAW2(2:2:end, 1:2:end) = RAW2(2:2:end, 1:2:end) - blks(3);
RAW2(2:2:end, 2:2:end) = RAW2(2:2:end, 2:2:end) - blks(4);
%}

RAW2(1:2:end, 1:2:end) = RAW2(1:2:end, 1:2:end) * gains(1);
RAW2(1:2:end, 2:2:end) = RAW2(1:2:end, 2:2:end) * gains(2);
RAW2(2:2:end, 1:2:end) = RAW2(2:2:end, 1:2:end) * gains(3);
RAW2(2:2:end, 2:2:end) = RAW2(2:2:end, 2:2:end) * gains(4);
% RAW2 = RAW2 + blk;
RGB2 = demosaic(uint16(RAW2), lower(bcm)); %figure,imshow(RGB2*gain*2^6,[])


imwrite(uint8(RGB2*gain), sprintf('%s__AWB_%.1f_%.1f_%.1f_%.1f__blk%.2f_gain%.4f_Demosaic_gauss2.bmp',filename(1:end-4),gains,blk,gain) )
imwrite(uint16(RGB2*gain*2^6), sprintf('%s__AWB_%.1f_%.1f_%.1f_%.1f__blk%.2f_gain%.4f_Demosaic.png',filename(1:end-4),gains,blk,gain) )



%}

