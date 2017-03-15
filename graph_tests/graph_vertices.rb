require 'redis'

redis = Redis.new

describe 'basic commands' do
  before do
    redis.flushdb
  end

  it "should list the graph vertices correctly" do
    redis.gvertex 'graph1', 'a'
    vertices = redis.gvertices 'graph1'
    vertices.should eq ['a']
    vertices = redis.gvertices 'graph1'
    vertices.should eq ['a']
  end
end
