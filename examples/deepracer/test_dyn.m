clear;
close all;
clc;
draw_arena();

% initialize
tau = 0.5;
Tmax = 5*tau;
u = [-0.8 1];
x0 = [-2.12 0 -pi/2 0];
draw_deepracer([x0(1) x0(2)], x0(3));

% simulate
x = x0;
state_log = [0 x0];
for k=1:(Tmax/tau)
    [t, x_posts]=ode45(@deepracer_ode,[0 tau], x, [],u);
    state_log = [state_log; (state_log(end,1)+t(2:end)) x_posts(2:end,1:2) wrapToPi(x_posts(2:end,3)) x_posts(2:end,4)];
    line([x(1) x_posts(end,1)],[x(2) x_posts(end,2)],'Color', 'red');
    x = x_posts(end,:);
    draw_deepracer([x(1) x(2)], x(3));
end

% plot
figure;
subplot(4,1,1);
plot(state_log(:,1),state_log(:,2),'LineWidth',1)
for tau_line = 0:tau:Tmax; xline(tau_line); end
ylabel('x-pos')
grid on;
subplot(4,1,2);
plot(state_log(:,1),state_log(:,3),'LineWidth',1)
for tau_line = 0:tau:Tmax; xline(tau_line); end
ylabel('y-pos')
grid on;
subplot(4,1,3);
plot(state_log(:,1),state_log(:,4),'LineWidth',1)
for tau_line = 0:tau:Tmax; xline(tau_line); end
ylabel('orientation (theta)')
grid on;
subplot(4,1,4);
plot(state_log(:,1),state_log(:,5),'LineWidth',1)
for tau_line = 0:tau:Tmax; xline(tau_line); end
ylabel('velocity')
xlabel('time')
grid on;


% darwing
function draw_arena()
    arena_width = 4.24;
    arena_hight = 4.24;
    draw_rectangle([0 0], arena_width, arena_hight, 0, 'k')
    grid on;
end
function draw_deepracer(base_location, angle)
    deepracer_width = 0.20;
    deepracer_hight = 0.48;
    dr_center_x = base_location(1) + (deepracer_hight/2)*cos(angle);
    dr_center_y = base_location(2) + (deepracer_hight/2)*sin(angle);
    
    angle = rad2deg(angle-pi/2);
    draw_rectangle([dr_center_x dr_center_y], deepracer_width, deepracer_hight, angle, 'b')
end
function draw_rectangle(center_location, L, H, deg, rgb)
    theta=deg*pi/180;
    center1=center_location(1);
    center2=center_location(2);
    R= ([cos(theta), -sin(theta); sin(theta), cos(theta)]);
    X=([-L/2, L/2, L/2, -L/2]);
    Y=([-H/2, -H/2, H/2, H/2]);
    for i=1:4
        T(:,i)=R*[X(i); Y(i)];
    end
    x_lower_left=center1+T(1,1);
    x_lower_right=center1+T(1,2);
    x_upper_right=center1+T(1,3);
    x_upper_left=center1+T(1,4);
    y_lower_left=center2+T(2,1);
    y_lower_right=center2+T(2,2);
    y_upper_right=center2+T(2,3);
    y_upper_left=center2+T(2,4);
    x_coor=[x_lower_left x_lower_right x_upper_right x_upper_left];
    y_coor=[y_lower_left y_lower_right y_upper_right y_upper_left];
    patch('Vertices',[x_coor; y_coor]','Faces',[1 2 3 4],'Edgecolor',rgb,'Facecolor','none','Linewidth',1.2);
    axis equal;
end


