require 'spec_helper'

describe Cause do
  before(:each) do
    @user = User.new :username => "Any user", :twitter_user_id => 1234, :email => 'a@ionatec.com.br'
    @user.stubs(:send_email)
    @user.save
    @valid_attributes = {
      :author => 'Any Author',
      :title => 'Any Title',
      :local => "Any Local",
      :abstract => "Any Abstract",
      :latitude => -30.06033724856420000,
      :longitude => -51.20349916931150000,
      :district => "Any District", 
      :user => @user
    }
  end
  
  context 'should validates the model' do
    it { Cause.new.should be_an_instance_of(Cause) }
    
    it 'should create a cause' do
      @cause = Cause.create! @valid_attributes
      @cause.should be_valid
    end
    
    it 'should require author, title, local, abstract, latitude, longitude, district' do
      [:author, :title, :local, :abstract, :latitude, :longitude, :district].each do |campo|
        @cause = Cause.new @valid_attributes
        @cause[campo] = nil
        @cause.should_not be_valid
      end
    end
  end
  
  context "there are some causes" do
    before(:each) do
      @user = User.create(:username => "Any user", :twitter_user_id => 1234) 
      cause1 = { :author => 'Any Author', :title => 'Any Title', :abstract => "Any abstract", :local => "Local1", :category_id => Category.first.id,
      :latitude => -30.06033724856420000, :longitude => -51.20349916931150000, :district => "Any District1",  :user => @user, :submited => "1" }
      cause2 = { :author => 'Any Author', :title => 'Any Title', :abstract => "Any abstract", :local => "Local2", :category_id => Category.all[2].id,
      :latitude => -30.06033724856420000, :longitude => -51.20349916931150000, :district => "Any District2",  :user => @user, :submited => "1" }
      cause3 = { :author => 'Any Author', :title => 'Any Title', :abstract => "Any abstract", :local => "Local2", :category_id => Category.all[2].id,
      :latitude => -30.06033724856420000, :longitude => -51.20349916931150000, :district => "Any District2",  :user => @user, :submited => "1" }
      cause4 = { :author => 'Any Author', :title => 'Any Title', :abstract => "Any abstract", :local => "Local2", :category_id => Category.all[2].id,
      :latitude => -30.06033724856420000, :longitude => -51.20349916931150000, :district => "Any District2",  :user => @user, :submited => "1" }
      cause5 = { :author => 'Any Author', :title => 'Any Title', :abstract => "Any abstract", :local => "Local2", :category_id => Category.all[2].id,
      :latitude => -30.06033724856420000, :longitude => -51.20349916931150000, :district => "Any District2",  :user => @user, :submited => "1" }
      
      @cause1 = Cause.create(cause1)
      @cause2 = Cause.create(cause2)
      @cause3 = Cause.create(cause3)
      @cause4 = Cause.create(cause4)
      @cause5 = Cause.create(cause5)
      
    end
    
    describe "related causes" do
      it "should return an empty array if there is no related cause" do
        @cause1.related_causes.should be_empty
      end
      
      it "should not return a cause from a different category" do
        @cause2.related_causes.should_not include(@cause1)
        @cause2.related_causes.should_not include(@cause2)
      end
      
      it "should return causes with same category" do
        @cause2.related_causes.should include(@cause3)
        @cause2.related_causes.should include(@cause4)
        @cause2.related_causes.should include(@cause5)
      end
    end
    
    describe "same_neighborhood_causes" do
      it "should return an empty array if there is no related cause" do
        @cause1.same_neighborhood_causes.should be_empty
      end
      
      it "should not return a cause in a different neighborhood" do
        @cause2.same_neighborhood_causes.should_not include(@cause1)
        @cause2.same_neighborhood_causes.should_not include(@cause2)
      end
      
      it "should return causes in same neighborhood" do
        @cause2.same_neighborhood_causes.should include(@cause3)
        @cause2.same_neighborhood_causes.should include(@cause4)
        @cause2.same_neighborhood_causes.should include(@cause5)
      end
    end    
  end

  context "FalaPoa integration" do

    it "should return valid informations from FalaPoa" do
      valid_info = { "status" => "success", "data" => {
        "status"       => "Aguardando informação",
        "destination"  => "SMIC - Licenciamento de Atividades Localizadas",
        "last_update"  => "2/3/2012 9:40:12",
        "service"      => "Estabelecimento sem alvará",
        "average_time" => "Vistoria e/ou fiscalização: 15 dias"
      }}
      cause = Cause.new @valid_attributes
      RestClient.stubs(:get).returns(valid_info.to_json.to_s)
      falapoa = cause.get_falapoa_data
      falapoa.should include valid_info["data"]
    end
  end
end
