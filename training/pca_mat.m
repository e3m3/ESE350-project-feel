FILEIN = 'datasets/parry-6,4,6.csv';

A = csvread(FILEIN)';
[K, N] = size(A);

mn = mean(A, 2);
A = A - repmat(mn, 1, N);

%%% Trying sliding window
window_size = 12;
A = conv2(A, ones(1, window_size), 'same');
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

%X = proj(1:3, :);
%[G, C] = kmeans(X, 2);
%clr = lines(K);
%figure, hold on
%scatter3(X(:,1), X(:,2), X(:,3), 36, clr(G,:), 'Marker','.')
%scatter3(C(:,1), C(:,2), C(:,3), 100, clr, 'Marker','o', 'LineWidth',3)
%hold off
%view(3), axis vis3d, box on, rotate3d on
%xlabel('x'), ylabel('y'), zlabel('z')

cl_labels = litekmeans(proj(1:2, :), 2);
cl_1 = proj(1:3, find(cl_labels == 1));
cl_2 = proj(1:3, find(cl_labels == 2));

b_1 = [min(cl_1(1, :)) max(cl_1(1, :)) min(cl_1(2, :)) max(cl_1(2, :))];
b_2 = [min(cl_2(1, :)) max(cl_2(1, :)) min(cl_2(2, :)) max(cl_2(2, :))];

figure;
hold on;
scatter(cl_1(1, :), cl_1(2, :), 'r');
scatter(cl_2(1, :), cl_2(2, :), 'b');

rectangle('Position', [b_1(1), b_1(3), b_1(2)-b_1(1), b_1(4)-b_1(3)], 'EdgeColor', 'r');
rectangle('Position', [b_2(1), b_2(3), b_2(2)-b_2(1), b_2(4)-b_2(3)], 'EdgeColor', 'b');

figure;
hold on;
scatter3(cl_1(1, :), cl_1(2, :), cl_1(3, :), 'r');
scatter3(cl_2(1, :), cl_2(2, :), cl_2(3, :), 'b');
