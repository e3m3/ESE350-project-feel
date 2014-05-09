FILEIN = 'new_datasets/parry-6,4,6.csv';
FILEOUT_PROJ = 'new_clusters/2-trans_4,6_projection.csv';
FILEOUT_CENT = 'new_clusters/2-trans_4,6_centroids.txt';

FILESET_1 = 'new_datasets/parry-4,6.csv';
FILESET_2 = 'new_datasets/parry-6,4.csv';

A = csvread(FILEIN)';
[K, N] = size(A);

mn = mean(A, 2);
A = A - repmat(mn, 1, N);

%%% Trying sliding window
%window_size = 12;
%A = conv2(A, ones(1, window_size), 'same');
%%%


Y = A'/sqrt(N - 1);
[u, S, PC] = svd(Y);

S = diag(S);
V = S.^2
norm_variances = V./norm(V, 1);
num_pc = sum(cumsum(norm_variances) < 0.90) + 1

proj = PC'*A;
figure;
scatter(proj(1, :), proj(2, :));

%figure;
%scatter3(proj(1, :), proj(2, :), proj(3, :));

%X = proj(1:3, :)';
%[G, C] = kmeans(X, 2, 'distance', 'sqEuclidean', 'start', 'sample');
%clr = lines(K);
%figure, hold on
%scatter3(X(:,1), X(:,2), X(:,3), 36, clr(G,:), 'Marker', '.')
%scatter3(C(:,1), C(:,2), C(:,3), 100, clr, 'Marker','o', 'LineWidth',3)
%hold off
%view(3), axis vis3d, box on, rotate3d on
%xlabel('x'), ylabel('y'), zlabel('z')

cl_labels = litekmeans(proj(1:2, :), 2);
cl_1 = proj(1:3, find(cl_labels == 1));
cl_2 = proj(1:3, find(cl_labels == 2));
%cl_3 = proj(1:3, find(cl_labels == 3));
%cl_4 = proj(1:3, find(cl_labels == 4));
%cl_5 = proj(1:3, find(cl_labels == 5));
%cl_6 = proj(1:3, find(cl_labels == 6));

b_1 = [min(cl_1(1, :)) max(cl_1(1, :)) min(cl_1(2, :)) max(cl_1(2, :))];
b_2 = [min(cl_2(1, :)) max(cl_2(1, :)) min(cl_2(2, :)) max(cl_2(2, :))];
%b_3 = [min(cl_3(1, :)) max(cl_3(1, :)) min(cl_3(2, :)) max(cl_3(2, :))];
%b_4 = [min(cl_4(1, :)) max(cl_4(1, :)) min(cl_4(2, :)) max(cl_4(2, :))];
%b_5 = [min(cl_5(1, :)) max(cl_5(1, :)) min(cl_5(2, :)) max(cl_5(2, :))];
%b_6 = [min(cl_6(1, :)) max(cl_6(1, :)) min(cl_6(2, :)) max(cl_6(2, :))];


% PLOT CLUSTER RESULTS
figure;
hold on;
scatter(cl_1(1, :), cl_1(2, :), 'r');
scatter(cl_2(1, :), cl_2(2, :), 'b');
%scatter(cl_3(1, :), cl_3(2, :), 'k');
%scatter(cl_4(1, :), cl_4(2, :), 'm');
%scatter(cl_5(1, :), cl_5(2, :), 'g');
%scatter(cl_6(1, :), cl_6(2, :), 'c');

rectangle('Position', [b_1(1), b_1(3), b_1(2)-b_1(1), b_1(4)-b_1(3)], 'EdgeColor', 'r');
rectangle('Position', [b_2(1), b_2(3), b_2(2)-b_2(1), b_2(4)-b_2(3)], 'EdgeColor', 'b');
%rectangle('Position', [b_3(1), b_3(3), b_3(2)-b_3(1), b_3(4)-b_3(3)], 'EdgeColor', 'k');
%rectangle('Position', [b_4(1), b_4(3), b_4(2)-b_4(1), b_4(4)-b_4(3)], 'EdgeColor', 'm');
%rectangle('Position', [b_5(1), b_5(3), b_5(2)-b_5(1), b_5(4)-b_5(3)], 'EdgeColor', 'g');
%rectangle('Position', [b_6(1), b_6(3), b_6(2)-b_6(1), b_6(4)-b_6(3)], 'EdgeColor', 'c');

%spread(proj(1:3, :), cl_labels);

mean_cl_1 = [mean(cl_1(1, :)), mean(cl_1(2, :)), mean(cl_1(3, :))];
mean_cl_2 = [mean(cl_2(1, :)), mean(cl_2(2, :)), mean(cl_2(3, :))];
scatter(mean_cl_1(1), mean_cl_1(2), 36, 'm');
scatter(mean_cl_2(1), mean_cl_2(2), 36, 'm');

figure;
hold on;
scatter3(cl_1(1, :), cl_1(2, :), cl_1(3, :), 'r');
scatter3(cl_2(1, :), cl_2(2, :), cl_2(3, :), 'b');
scatter3(mean_cl_1(1), mean_cl_1(2), mean_cl_1(3), 36, 'm');
scatter3(mean_cl_2(1), mean_cl_2(2), mean_cl_2(3), 36, 'm');
%scatter3(cl_3(1, :), cl_3(2, :), cl_3(3, :), 'k');
%scatter3(cl_4(1, :), cl_4(2, :), cl_4(3, :), 'm');
%scatter3(cl_5(1, :), cl_5(2, :), cl_5(3, :), 'g');
%scatter3(cl_6(1, :), cl_6(2, :), cl_6(3, :), 'c');


% DETERMINE CLUSTER CENTRIODS AND WRITE TO FILE
csvwrite(FILEOUT_PROJ, PC');

D1 = csvread(FILESET_1)';
D2 = csvread(FILESET_2)';
mn1 = mean(D1, 2);
mn2 = mean(D2, 2);
D1 = D1 - repmat(mn1, 1, size(D1, 2));
D2 = D2 - repmat(mn2, 1, size(D2, 2));
D1 = conv2(D1, ones(1, window_size), 'same');
D2 = conv2(D2, ones(1, window_size), 'same');
P1 = PC'*D1;
P2 = PC'*D2;
centroid_1 = [mean(P1(1, :)), mean(P1(2, :)), mean(P1(3, :))];
centroid_2 = [mean(P2(1, :)), mean(P2(2, :)), mean(P2(3, :))];

dist = @(p1, p2) (sqrt((p2(1) - p1(1))^2 + (p2(2) - p1(2))^2 + (p2(3) - p1(3))^2));
dist1 = dist(centroid_1, mean_cl_1(1:3));
dist2 = dist(centroid_1, mean_cl_2(1:3));
if dist1 < dist2
  tag1 = '4_6';
  tag2 = '6_4';
else
  tag2 = '4_6';
  tag1 = '6_4';
end
centroid_file = fopen(FILEOUT_CENT, 'wt');
fprintf(centroid_file, '%f,%f,%f,%s\n', mean_cl_1(1), mean_cl_1(2), mean_cl_1(3), tag1);
fprintf(centroid_file, '%f,%f,%f,%s\n', mean_cl_2(1), mean_cl_2(2), mean_cl_2(3), tag2);
%fwrite(centroid_file, [num2str(centroid_1(1)) ',' num2str(centroid_1(2)) ',' tag1 '\r\n']);
%fwrite(centroid_file, [num2str(centroid_2(1)) ',' num2str(centroid_2(2)) ',' tag2 '\r\n']);
fclose(centroid_file);
