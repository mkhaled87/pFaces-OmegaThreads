% deepracer system dynamics
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
