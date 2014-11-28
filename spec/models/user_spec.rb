require 'spec_helper'

describe User do
  before(:each) do
    @valid_attributes = {
      :username => 'Any User',
      :twitter_user_id => 123,
      :email => 'a@ionatec.com.br'
    }
  end
  
  context 'should validates the model' do
    it { User.new.should be_an_instance_of(User) }

    it 'should create a user' do
      @user = User.new @valid_attributes
      @user.stubs(:send_email)
      @user.save
      @user.should be_valid
    end

    it 'should require unique username' do
      @user1 = User.new @valid_attributes
      @user1.stubs(:send_email)
      @user1.save
      @user2 = User.new @valid_attributes
      @user2.should_not be_valid
    end

    it 'should require username' do
      @user = User.new 
      @user.should_not be_valid
    end

    it 'should initialize level with 0' do
      @user = User.new
      @user.level.should eql(0)
    end

    it 'should require one social network' do
      @user = User.new :username => "oio" 
      @user.should_not be_valid
    end

    it 'should require only one social network' do
      @user = User.new @valid_attributes
      @user.google_email = "aaa@gmail.com"
      @user.should_not be_valid
    end

    it 'should require unique twitter' do
      @user1 = User.new @valid_attributes
      @user1.stubs(:send_email)
      @user1.save
      @user2 = User.new :username => "oio", :twitter_user_id => 123 
      @user2.should_not be_valid
    end

    it 'should require unique google email' do
      @user1 = User.create :username => "Any username", :google_email => "aaaa@gmail.com"
      @user2 = User.new :username => "oio", :google_email => "aaaa@gmail.com" 
      @user2.should_not be_valid
    end

    it 'should require unique facebook' do
      @user1 = User.create :username => "Any username", :facebook_id => "1234" 
      @user2 = User.new  :username => "oio", :facebook_id => "1234"
      @user2.should_not be_valid
    end
  end
end
