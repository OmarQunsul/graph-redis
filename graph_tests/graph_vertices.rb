require 'redis'

redis = Redis.new

describe 'basic commands' do
  before do
    redis.flushdb
  end

  it "should list the graph vertices correctly" do
    redis.gvertex 'graph1', 'a'
    vertices = redis.gvertices 'graph1', '*'
    vertices.should eq ['a']
    vertices = redis.gvertices 'graph1', '*'
    vertices.should eq ['a']
  end

  it "should be able to filter graph vertices" do
    redis.gvertex 'graph1', 'country:jordan', 'country:germany', 'user:omar', 'user:john'
    countries = redis.gvertices 'graph1', 'country:*'
    expect(countries).to eq(%w(country:jordan country:germany))
    countries = redis.gvertices 'graph1', 'user:*'
    expect(countries).to eq(%w(user:omar user:john))
  end
end
