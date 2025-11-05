cd /Users/jameshan/CSE/CSE240/UCSD_CSE240A_F25_BP/src
make
# bunzip2 -kc /Users/jameshan/CSE/CSE240/UCSD_CSE240A_F25_BP/traces/U1_Blender.bz2 | ./predictor --static > ../results/static/U1.txt
# bunzip2 -kc /Users/jameshan/CSE/CSE240/UCSD_CSE240A_F25_BP/traces/U2_Leela.bz2 | ./predictor --static > ../results/static/U2.txt
# bunzip2 -kc /Users/jameshan/CSE/CSE240/UCSD_CSE240A_F25_BP/traces/U3_GCC.bz2 | ./predictor --static > ../results/static/U3.txt
# bunzip2 -kc /Users/jameshan/CSE/CSE240/UCSD_CSE240A_F25_BP/traces/U4_Cam4.bz2 | ./predictor --static > ../results/static/U4.txt

# bunzip2 -kc /Users/jameshan/CSE/CSE240/UCSD_CSE240A_F25_BP/traces/U1_Blender.bz2 | ./predictor --gshare > ../results/gshare/U1.txt
# bunzip2 -kc /Users/jameshan/CSE/CSE240/UCSD_CSE240A_F25_BP/traces/U2_Leela.bz2 | ./predictor --gshare > ../results/gshare/U2.txt
# bunzip2 -kc /Users/jameshan/CSE/CSE240/UCSD_CSE240A_F25_BP/traces/U3_GCC.bz2 | ./predictor --gshare > ../results/gshare/U3.txt
# bunzip2 -kc /Users/jameshan/CSE/CSE240/UCSD_CSE240A_F25_BP/traces/U4_Cam4.bz2 | ./predictor --gshare > ../results/gshare/U4.txt

bunzip2 -kc /Users/jameshan/CSE/CSE240/UCSD_CSE240A_F25_BP/traces/U1_Blender.bz2 | ./predictor --tournament > ../results/tournament/U1.txt
bunzip2 -kc /Users/jameshan/CSE/CSE240/UCSD_CSE240A_F25_BP/traces/U2_Leela.bz2 | ./predictor --tournament > ../results/tournament/U2.txt
bunzip2 -kc /Users/jameshan/CSE/CSE240/UCSD_CSE240A_F25_BP/traces/U3_GCC.bz2 | ./predictor --tournament > ../results/tournament/U3.txt
bunzip2 -kc /Users/jameshan/CSE/CSE240/UCSD_CSE240A_F25_BP/traces/U4_Cam4.bz2 | ./predictor --tournament > ../results/tournament/U4.txt

# bunzip2 -kc /Users/jameshan/CSE/CSE240/UCSD_CSE240A_F25_BP/traces/U1_Blender.bz2 | ./predictor --custom > ../results/custom/U1.txt
# bunzip2 -kc /Users/jameshan/CSE/CSE240/UCSD_CSE240A_F25_BP/traces/U2_Leela.bz2 | ./predictor --custom > ../results/custom/U2.txt
# bunzip2 -kc /Users/jameshan/CSE/CSE240/UCSD_CSE240A_F25_BP/traces/U3_GCC.bz2 | ./predictor --custom > ../results/custom/U3.txt
# bunzip2 -kc /Users/jameshan/CSE/CSE240/UCSD_CSE240A_F25_BP/traces/U4_Cam4.bz2 | ./predictor --custom > ../results/custom/U4.txt
