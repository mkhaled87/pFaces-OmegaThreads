clear;
close all;
clc;
draw_arena();

% initialize
tau = 0.5;
Tmax = 20*tau;
u = [1.0 3];
x0 = [2.12 0 pi/2 0];
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


%% functions

% deepracer system dynamics
function [a,b] = get_v_params(u_speed)
    if u_speed == 0.0
        K = 1.5; T = 0.25;
    elseif u_speed == 0.45
        K = 1.9353; T = 0.9092;
    elseif u_speed == 0.50
        K = 2.2458; T = 0.8942;
    elseif u_speed == 0.55
        K = 2.8922; T = 0.8508;
    elseif u_speed == 0.60
        K = 3.0332; T = 0.8642;
    elseif u_speed == 0.65
        K = 3.1274; T = 0.8419;
    elseif u_speed == 0.70
        K = 3.7112; T = 0.8822;
    elseif u_speed == -0.45
        K = 1.6392; T = 1.3008;
    elseif u_speed == -0.50
        K = 2.5998; T = 0.9882;
    elseif u_speed == -0.55
        K = 2.8032; T = 0.9640;
    elseif u_speed == -0.60
        K = 3.1457; T = 0.9741;
    elseif u_speed == -0.65
        K = 3.6170; T = 0.9481;
    elseif u_speed == -0.70
        K = 3.6391; T = 0.9285;
    else
        error('Invalid input !');
    end
    a = -1/T;
    b = K/T;
end
function psi = map_steering(angle_in)
    p1 = -0.116;
    p2 = -0.02581;
    p3 = 0.3895;
    p4 = 0.02972;
    x = angle_in;
    psi = p1*x^3 + p2*x^2 + p3*x + p4;
end
function s = map_speed(speed_in)
    switch speed_in
		case  6 
            s = 0.70;
		case  5
            s = 0.65;
		case  4
            s = 0.60;
		case  3
            s = 0.55;
		case  2
            s = 0.50;
		case  1
            s = 0.45;
		case  0
            s = 0.00;
		case -1
            s = -0.45;
		case -2
            s = -0.50;
		case -3
            s = -0.55;
		case -4
            s = -0.60;
		case -5
            s = -0.65;
		case -6
            s = -0.70;
    end
end
function dxdt = deepracer_ode(~,x,u)
    dxdt = zeros(4,1);
    u_steer = map_steering(u(1));
	u_speed = map_speed(u(2));
    L = 0.165;
    [a,b] = get_v_params(u_speed);
    
	dxdt(1) = x(4)*cos(x(3));
	dxdt(2) = x(4)*sin(x(3));
	dxdt(3) = (x(4)/L)*tan(u_steer);
	dxdt(4) = a*x(4) + b*u_speed;
end

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


