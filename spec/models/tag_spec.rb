require 'spec_helper'

describe Tag do
  before(:each) do
    @valid_attributes = {
      :name => 'Any Tag'
    }
  end

  context 'should validates the model' do
    it { Tag.new.should be_an_instance_of(Tag) }

    it 'should create a tag' do
      @tag = Tag.create @valid_attributes
      @tag.should be_valid
    end
  end

end
