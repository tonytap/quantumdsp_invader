function dirac()
    fs = 48000;
    ir = zeros(fs/2, 1);
    ir(1) = 1;
    plot(ir);
    audiowrite("irs/Off.wav", ir, fs);
end