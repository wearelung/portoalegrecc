require 'spec_helper'

describe UsersController do
  before(:each) do
    @user = User.first
  end
  
  it "should register username on session when signup with valid user" do
    user = User.new :username => "oioi"
    get :new, :format => :json, :username => user.username
    data = Hash.new
    data['username'] = user.username
    data['success'] = true
    session[:user].should eql(user.username)
    session[:type].should eql("signup")
    response.body.should eql(data.to_json)
  end
  
  it "should return to auth page with invalid user" do
    get :new, :format => :json, :username => @user.username
    data = Hash.new
    data['username'] = @user.username
    data['success'] = false
    session[:user].should be_nil
    session[:type].should be_nil
    response.body.should eql(data.to_json)
  end
  
  it "should return to root page when signup via twitter with already registered account" do
    user = User.find_by_twitter_user_id 78
    session[:user] = user.username
    session[:type] = "signup"
    request.env["omniauth.auth"] = { 
      'provider' => "twitter",
      'uid' => user.twitter_user_id,
      'info' => { 
        'name' => user.username, 
        'image' => user.profile_image_url,
        'location' => user.location
      },
      'extra' => { 'raw_info' => { 'screen_name' => user.twitter_username } }
    }
    get :create
    flash[:notice].should eql(I18n.t "user.notices.twitter_already_registered")
    session[:user].should be_nil
    response.should redirect_to(root_path)
  end
  
  it "should create a new user when signup with a new twitter account" do
    user = User.new :username => "maisum"
    session[:user] = user.username
    session[:type] = "signup"
    request.env["omniauth.auth"] = { 
      'provider' => "twitter",
      'uid' => 79,
      'info' => { 
        'name' => user.username, 
        'image' => "img",
        'location' => "loc"
      },
      'extra' => { 'raw_info' => { 'screen_name' => 'outro' } }
    }
    get :create
    response.should redirect_to(root_path)
  end
  
  #it "should return to auth page when signup via twitter with invalid account" do

  it "should return to root page when signup via google email with already registered account" do
    user = User.find_by_google_email "rviana@ionatec.com.br"
    session[:user] = user.username
    session[:type] = "signup"
    request.env["omniauth.auth"] = { 
      'provider' => "google",
      'info' => { 
        'name' => user.username, 
        'email' => user.google_email
      }
    }
    get :create
    flash[:notice].should eql(I18n.t "user.notices.google_email_already_registered")
    session[:user].should be_nil
    response.should redirect_to(root_path)
  end
  
  it "should create a new user when signup with a new google email account" do
    username = "maisum"
    session[:user] = username
    session[:type] = "signup"
    request.env["omniauth.auth"] = { 
      'provider' => "google",
      'info' => { 
        'name' => username,
        'email' => "maisum@ionatec.com.br"
      }
    }
    get :create
    response.should redirect_to(root_path)
  end
  
  #it "should return to auth page when signup via google email with invalid account" do

  it "should return to root page when signup via facebook with already registered account" do
    user = User.find_by_facebook_id "98765"
    session[:user] = user.username
    session[:type] = "signup"
    request.env["omniauth.auth"] = { 
      'provider' => "facebook",
      'uid' => user.facebook_id,
      'info' => { 
        'name' => user.username, 
        'image' => user.profile_image_url,
        'location' => user.location
      }
    }
    get :create
    flash[:notice].should eql(I18n.t "user.notices.facebook_already_registered")
    session[:user].should be_nil
    response.should redirect_to(root_path)
  end
  
  it "should create a new user when signup with a new facebook account" do
    user = User.new :username => "maisum"
    session[:user] = user.username
    session[:type] = "signup"
    request.env["omniauth.auth"] = { 
      'provider' => "facebook",
      'uid' => "newfacebook@gmail.com",
      'info' => { 
        'name' => user.username, 
        'image' => "img",
        'location' => "loc"
      }
    }
    get :create
    response.should redirect_to(root_path)
  end
  
  #it "should return to auth page when signup via facebook with invalid account" do
    
  it "should return to auth page when login with incorrect twitter account" do
    user = User.find_by_twitter_user_id 78
    session[:us] = user.username
    session[:type] = "login"
    request.env["omniauth.auth"] = { 
      'provider' => "twitter",
      'uid' => '80'
    }
    get :create
    flash[:notice].should eql(I18n.t "user.notices.twitter_incorrect_associated")
    response.should redirect_to(root_path)
  end
  
  it "should redirect to index page when username and twitter are correct associated" do
    user = User.find_by_twitter_user_id 78
    session[:us] = user.username
    session[:type] = "login"
    request.env["omniauth.auth"] = { 
      'provider' => "twitter",
      'uid' => user.twitter_user_id.to_s,
      'extra' => { 'raw_info' => { 'screen_name' => user.twitter_username } }
    }
    get :create
    response.should redirect_to(root_path)
  end

  it "should return to auth page when login with incorrect google email" do
    user = User.find_by_google_email "rviana@ionatec.com.br"
    session[:us] = user.username
    session[:type] = "login"
    request.env["omniauth.auth"] = { 
      'provider' => "google",
      'info' => {
        'email' => "outro@gmail.com"
      }
    }
    get :create
    flash[:notice].should eql(I18n.t "user.notices.google_email_incorrect_associated")
    response.should redirect_to(root_path)
  end
  
  it "should redirect to index page when username and google email are correct associated" do
    user = User.find_by_google_email "rviana@ionatec.com.br"
    session[:us] = user.username
    session[:type] = "login"
    request.env["omniauth.auth"] = { 
      'provider' => "google",
      'info' => {
        'email' => user.google_email
      }
    }
    get :create
    response.should redirect_to(root_path)
  end
  
  
  it "should return to auth page when login with incorrect facebook account" do
    user = User.find_by_facebook_id "98765"
    session[:us] = user.username
    session[:type] = "login"
    request.env["omniauth.auth"] = { 
      'provider' => "facebook",
      'uid' => "maisum@gmail.com"
    }
    get :create
    flash[:notice].should eql(I18n.t "user.notices.facebook_incorrect_associated")
    response.should redirect_to(root_path)
  end
  
  it "should redirect to index page when username and facebook are correct associated" do
    user = User.find_by_facebook_id "98765"
    session[:us] = user.username
    session[:type] = "login"
    request.env["omniauth.auth"] = { 
      'provider' => "facebook",
      'uid' => user.facebook_id
    }
    get :create
    response.should redirect_to(root_path)
  end

end
