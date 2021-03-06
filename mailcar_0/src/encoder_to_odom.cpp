  //this class is to implement the transformation from encoder data to encoder data that ROS can uses
  //encoder is the data sent from mbed to ROS, where x is the count of left wheel encoder
  //, and y is the count of right wheel encoder
  //next will calculate the position of the car in the world based on the encoder info
  //    x_pose is the x position corresponding to the fixed fram
  //    y_pose is the y position corresponding to the fixed frame
  //    theta is the angle of x direction between the car's frame and the fixed frame.

#include <ros/ros.h>
//#include <geometry_msgs/Twist.h>
#include <geometry_msgs/Quaternion.h>
#include <geometry_msgs/PointStamped.h>
#include <tf/tf.h>
#include <tf/transform_broadcaster.h>
//#include <std_msgs/Int8.h>
#include <signal.h>
#include <termios.h>
#include <stdio.h>
#include <math.h>

#define WHEEL_BASE 0.234 ///21cm
#define DISTANCE_PER_TICK 0.0098 //0.98cm per tick
#define PI 3.1415926
/*
class TeleopCar
{
public:
  TeleopCar();
  void keyLoop();

private:
  ros::NodeHandle nh_;
  std_msgs::Int8 velocity;
  ros::Publisher twist_pub_;
  
};
*/

class EncoderOdom
{
public:
  EncoderOdom();
  ros::Subscriber encoder_sub_;
  void readEncoder(const geometry_msgs::PointStamped& encoder);
  tf::TransformBroadcaster odom_broad_;
  tf::TransformBroadcaster odom_broad_test_;

private:
  ros::NodeHandle nh_;
  geometry_msgs::TransformStamped t_;
  geometry_msgs::TransformStamped test_;
  std::string base_link;
  std::string odom;
  std::string test_from;
  std::string test_to;
  double x_pose;
  double y_pose;
  double theta;
};

/*
TeleopCar::TeleopCar()
{
  velocity.data = 0;
  twist_pub_ = nh_.advertise<std_msgs::Int8>("car/cmd_vel", 1);
}
*/


EncoderOdom::EncoderOdom()
{
  puts("running encoder to odom:");
  base_link = "/base_footprint";
  odom = "/odom";
  test_from = "/from";
  test_to = "/to";
  encoder_sub_ = nh_.subscribe("/car/encoder",100,&EncoderOdom::readEncoder, this);
  t_.header.seq = 0;
  t_.header.frame_id = odom;
  t_.header.stamp.sec = 0;
  t_.header.stamp.nsec = 0;
  t_.child_frame_id = base_link;
  t_.transform.translation.x = 0;
  t_.transform.translation.y = 0;
  t_.transform.translation.z = 0;
  t_.transform.rotation = tf::createQuaternionMsgFromYaw(0);
  test_.header.seq = 0;
  test_.header.frame_id = test_from;
  test_.header.stamp.sec = 0;
  test_.header.stamp.nsec = 0;
  test_.child_frame_id = test_to;
  test_.transform.translation.x = 0;
  test_.transform.translation.y = 0;
  test_.transform.translation.z = 0;
  test_.transform.rotation = tf::createQuaternionMsgFromYaw(0);
  x_pose = 0;
  y_pose = 0;
  theta = 0;
  odom_broad_.sendTransform(t_);  
  odom_broad_test_.sendTransform(test_);
}

int kfd = 0;
struct termios cooked, raw;

void quit(int sig)
{
  tcsetattr(kfd, TCSANOW, &cooked);
  ros::shutdown();
  exit(0);
}

/*
int main(int argc, char** argv)
{
  ros::init(argc, argv, "teleop_car");
  TeleopCar teleop_car;

  signal(SIGINT,quit);

  teleop_car.keyLoop();
  
  return(0);
}
*/

int main(int argc, char** argv)
{
  ros::init(argc, argv, "encoder_tf_odom");
  EncoderOdom encoder_tf_odom;

  // get the console in raw mode
  tcgetattr(kfd, &cooked);
  memcpy(&raw, &cooked, sizeof(struct termios));
  raw.c_lflag &=~ (ICANON | ECHO);
  // Setting a new line, then end of file
  raw.c_cc[VEOL] = 1;
  raw.c_cc[VEOF] = 2;
  tcsetattr(kfd, TCSANOW, &raw);


  signal(SIGINT,quit);

  ros::spin();
  
  return(0);
}


void EncoderOdom::readEncoder(const geometry_msgs::PointStamped& encoder)
{
  double delta_distance;
  double delta_theta;
  //encoder is the data sent from mbed to ROS, where x is the count of left wheel encoder
  //, and y is the count of right wheel encoder
  //next will calculate the position of the car in the world based on the encoder info
  //    x_pose is the x position corresponding to the fixed frame
  //    y_pose is the y position corresponding to the fixed frame
  //    theta is the angle of x direction between the car's frame and the fixed frame.
  delta_distance = (encoder.point.x + encoder.point.y)*DISTANCE_PER_TICK/2;
  delta_theta = (encoder.point.y - encoder.point.x)*DISTANCE_PER_TICK/WHEEL_BASE;
  theta += delta_theta;
  theta -= (double)((int)(theta/2/PI))*2*PI;
  x_pose += delta_distance * cos(theta);
  y_pose += delta_distance * sin(theta);
  t_.header.seq = encoder.header.seq;
  t_.header.stamp = ros::Time::now();
  t_.transform.translation.x = x_pose;
  t_.transform.translation.y = y_pose;
  t_.transform.rotation = tf::createQuaternionMsgFromYaw (theta);

  //test_ here is for debug, to see if the data from encoder is correctly received by this method.
  test_.header.seq = encoder.header.seq;
  test_.header.stamp = ros::Time::now();
  test_.transform.translation.x = encoder.point.x;
  test_.transform.translation.y = encoder.point.y;
  test_.transform.rotation = tf::createQuaternionMsgFromYaw (theta);
  odom_broad_.sendTransform(t_);
  odom_broad_test_.sendTransform(test_);
  ros::spinOnce();
}

