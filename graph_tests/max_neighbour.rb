require 'redis'

redis = Redis.new

describe 'max_neighbour' do
  before do
    redis.flushdb
    redis.gvertex 'graph1', 'a', 'b', 'c', 'd', 'e'
    redis.gedge 'graph1', 'a', 'b', 2
    redis.gedge 'graph1', 'a', 'c', 1
    redis.gedge 'graph1', 'a', 'd', 3

    redis.gedge 'graph1', 'd', 'c', 1
  end

  it "should return 0 for isolated vertex" do
    value = redis.gmaxneighbour('graph1', 'e')
    expect(value).to eq(0)
  end

  it "should return the correct value for multi neighbour" do
    arr = redis.gmaxneighbour('graph1', 'a')
    expect(arr).to eq(['d', 3])
    redis.gedge 'graph1', 'a', 'c', 20
    arr = redis.gmaxneighbour('graph1', 'a')
    expect(arr).to eq(['c', 20])
  end

  it "should return the correct value for single neighbour" do
    arr = redis.gmaxneighbour('graph1', 'b')
    expect(arr).to eq(['a', 2])
  end
end
