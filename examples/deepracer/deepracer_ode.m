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
        K = 0.0; T = 0.25;
    elseif u_speed == 0.45
        K = 1.9953; T = 0.9933;
    elseif u_speed == 0.50
        K = 2.3567; T = 0.8943;
    elseif u_speed == 0.55
        K = 3.0797; T = 0.88976;
    elseif u_speed == 0.60
        K = 3.2019; T = 0.87595;
    elseif u_speed == 0.65
        K = 3.3276; T = 0.89594;
    elseif u_speed == 0.70
        K = 3.7645; T = 0.92501;
    elseif u_speed == -0.45
        K = 1.8229; T = 1.8431;
    elseif u_speed == -0.50
        K = 2.3833; T = 1.2721;
    elseif u_speed == -0.55
        K = 2.512; T = 1.1403;
    elseif u_speed == -0.60
        K = 3.0956; T = 1.12781;
    elseif u_speed == -0.65
        K = 3.55; T = 1.1226;
    elseif u_speed == -0.70
        K = 3.6423; T = 1.1539;
    else
        error('Invalid input !');
    end
    a = -1/T;
    b = K/T;
end
function psi = map_steering(angle_in)
    p1 = -0.1167;
    p2 = 0.01949;
    p3 = 0.3828;
    p4 = -0.0293;
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
