require 'spec_helper'

describe Category do
  before(:each) do
    @valid_attributes = {
      :name => 'Any Category'
    }
  end

  context 'should validates the model' do
    it { Category.new.should be_an_instance_of(Category) }

    it 'should create a category' do
      @category = Category.create @valid_attributes
      @category.should be_valid
    end
  end

end
