
%1. comment out the header
fid = fopen('john_cond2_0_0_1.dat', 'rt+');
counter = 0;
header = 25;  %number of lines in the header

if ( '%' ~= fread(fid, 1))  %if file was not modified
while 1
    if( counter < header)
      fseek(fid,-1,'cof');
      fprintf(fid,'%%');
    end
    tline = fgets(fid);
    if ~ischar(tline),   break,   end
    counter = counter +1;
end
end
fclose(fid)

%2. now do data processing
data = load('john_cond2_0_0_1.dat');
size(data)
